/*
 * This is the LED driver API for AaltoNeppi2018. It abstracts away RIOT APA102 LED strip
 * drivers.
 *
 * Made by Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is public domain.
 */
#include <stdio.h>
#include <string.h>

#include "leds.h"
#include "apa102.h"
#include "apa102_params.h"
#define ENABLE_DEBUG                (1)
#include "debug.h"

#define LEDS_MESSAGE_QUEUE_SIZE     (8)

/**
 * Message type for color cycle control.
 */
#define MESSAGE_COLOR_CYCLE         (213)

/**
 * The amount of micro seconds a single step on hsv circle takes
 */
#define CYCLE_TIMER_US              (500000)

static kernel_pid_t led_pid;
static kernel_pid_t main_pid;

static apa102_t dev;
static xtimer_t cycle_timer;
static msg_t leds_rcv_queue[LEDS_MESSAGE_QUEUE_SIZE];

/**
 * Flag to hold active/sleep state
 */
static uint8_t active;

/**
 * Flag to hold cycle/hold color state
 */
static uint8_t cycling;

/**
 * Place to hold brightness value while LEDS are in sleep state
 */
static float last_intensity;

/**
 * Simple color cycle function. It advance the color by 1 degree on the
 * hsv color circle.
 */
static color_hsv_t cycle_colors(color_hsv_t hsv)
{
    const float HUE_STEP = 2.0f;
    const float HUE_FULL = 360.0f;

    hsv.h += HUE_STEP;
    if (hsv.h > HUE_FULL) {
        hsv.h = 0;
    }
    return hsv;
}

/**
 * Internal function that actually sets the LED colors.
 *
 * Function also tells main what color and intensity is currently active. Color
 * is sent as hue (0-360), while intensity is sent as an alpha value (0-255).
 */
static void leds_internal_set_color(color_hsv_t *led_color)
{
    color_rgb_t temp_color;
    color_hsv2rgb(led_color, &temp_color);

    color_rgba_t leds[APA102_PARAM_LED_NUMOF];
    for (int i = 0; i < APA102_PARAM_LED_NUMOF; i++) {
        leds[i].color = temp_color;
	leds[i].alpha = 255;
    }
    // This is the RIOT apa102 driver function that sets the color.
    apa102_load_rgba(&dev, leds);
    // Now that the color is set, we need to inform the main about the new
    // color and it's intensity.
    color_hsv_t temp_hsv;
    color_rgb2hsv(&(temp_color), &temp_hsv);

    msg_t m = {
        .type = MESSAGE_COLOR_NEW_HUE,
        .content = { .value = led_color->h, },
    };
    msg_try_send(&m, main_pid);

    if (last_intensity != led_color->v) {
        last_intensity = led_color->v;
        msg_t m = {
            .type = MESSAGE_COLOR_NEW_VALUE,
            .content = { .value = led_color->v * 100 },
        };
        msg_try_send(&m, main_pid);
    }
}


NORETURN static void *led_thread(void *arg)
{
    (void) arg;
    DEBUG("LED thread started, pid: %" PRIkernel_pid "\n", thread_getpid());
    //initialize the message queue
    msg_init_queue(leds_rcv_queue,LEDS_MESSAGE_QUEUE_SIZE);
    //initialize apa102.
    apa102_init(&dev, apa102_params);
    color_hsv_t led_color;
    msg_t m;
    // m_cycle is used by the timer that causes colors to cycle
    msg_t m_cycle;
    m_cycle.type = MESSAGE_COLOR_CYCLE;
    // The loop will wait to receive any color messages and process them.
    for (;;) {
        msg_receive(&m);
        switch (m.type) {
            case MESSAGE_COLOR_NEW:
                // This changes the LEDs to the given color.
                led_color = *(color_hsv_t *)m.content.ptr;
                leds_internal_set_color(&led_color);
                break;
            case MESSAGE_COLOR_SET_HOLD:
                // This means we start holding the current color/stop cycling.
                cycling = 0;
                break;
            case MESSAGE_COLOR_SET_CYCLE:
                // This starts the color cycle process.
                // To prevent starting multiple timers, we ignore
                // repeated messages.
                if (cycling) {
                    continue;
                }
                cycling = 1;
                // FALLTHROUGH
            case MESSAGE_COLOR_CYCLE:
                // We execute a loop every 250ms, until set to hold.
                if (cycling) {
                    led_color = cycle_colors(led_color);
                    leds_internal_set_color(&led_color);
                    // And we start the next cycle.
                    xtimer_set_msg(&cycle_timer, CYCLE_TIMER_US, &m_cycle, led_pid);
                }
                break;
            case MESSAGE_COLOR_INTENSITY:
                // The device is active again, we need to refresh the intensity.
                active = 1;
                led_color.v = (float)m.content.value / 10000 /*XXX*/;
                leds_internal_set_color(&led_color);
                break;
            default:
                break;
        }
    }
    /* NOTREACHED */
}

/**
 * API function to initialize the LEDs
 */
void leds_init(kernel_pid_t pid)
{
    main_pid = pid;
    static char led_thread_stack[THREAD_STACKSIZE_DEFAULT * 2];
    led_pid = thread_create(led_thread_stack, sizeof(led_thread_stack),
                            THREAD_PRIORITY_MAIN - 1, 0, led_thread, NULL, "led_thread");
}

void leds_set_color(const color_hsv_t *led_color)
{
    msg_t m = {
	.type = MESSAGE_COLOR_NEW,
	.content = { .ptr = (void*)led_color },
    };
    msg_try_send(&m, led_pid);
}

void leds_set_intensity(float intensity)
{
    msg_t m = {
        .type = MESSAGE_COLOR_INTENSITY,
        .content = { .value = intensity * 10000/*XXX*/, },
    };
    msg_send(&m, led_pid);
}

void leds_cycle(void)
{
    msg_t m;
    m.type = MESSAGE_COLOR_SET_CYCLE;
    msg_send(&m, led_pid);
}

void leds_hold(void)
{
    msg_t m;
    m.type = MESSAGE_COLOR_SET_HOLD;
    msg_send(&m, led_pid);
}
