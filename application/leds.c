/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is public domain.
 */
#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "leds.h"

static kernel_pid_t led_pid;

void led_set_color(color_rgba_t led_color)
{
    /*
     * The apa102 driver wants a color_rgba_t input. Problem is,
     * even though we have it, RIOT messages are uint32_t, so we have
     * to pack the colors into a single value, only to break them up again.
     */
    msg_t m;
    //Message needs to be set to 0. Otherwise garbage appears.
    memset(&m, 0, sizeof(msg_t));
    //pack the values and send them off. Maybe you could cast a color_rgba_t into
    //uint32_t and send it, but I'm not sure about the compiler padding.
    m.content.value |= ((uint32_t)led_color.alpha << 24);
    m.content.value |= ((uint32_t)led_color.color.b << 16);
    m.content.value |= ((uint32_t)led_color.color.g << 8);
    m.content.value |= ((uint32_t)led_color.color.r);
    //printf("%d\n",m.content.value);
    //try_send because we want this function to be non-blocking
    msg_try_send(&m, led_pid);
}


void *led_thread(void *arg)
{
    (void) arg;
    //initialize the message queue
    msg_t rcv_queue[4];
    msg_init_queue(rcv_queue,4);
    //initialize apa102
    apa102_t dev;
    apa102_init(&dev, &apa102_params[0]);

    color_rgba_t led_color;
    color_rgba_t leds[APA102_PARAM_LED_NUMOF];
    msg_t m;
    while (1)
    {
        //We now wait to receive a 32-bit package that we need to parse
        //back into colors.
        msg_receive(&m);
        //red mask is 0x000000ff
        led_color.color.r = m.content.value & 0x000000ff;
        //green mask is 0x0000ff00
        led_color.color.g = ((m.content.value & 0x0000ff00) >> 8);
        //blue mask is 0x00ff0000
        led_color.color.b = ((m.content.value & 0x00ff0000) >> 16);
        //alpha mask is 0xff000000
        led_color.alpha = ((m.content.value & 0xff000000) >> 24);
        /*
        printf("%d\n",led_color.color.r);
        printf("%d\n",led_color.color.g);
        printf("%d\n",led_color.color.b);
        printf("%d\n",led_color.alpha);
        */
        //apa102_load_rgba wants an array of colors for each leds, so
        //we copy the same color to each index
        for (int i = 0; i < APA102_PARAM_LED_NUMOF; i++)
        {
            leds[i] = led_color;
        }
        //Fire it off! The drivers will do the rest
        apa102_load_rgba(&dev,leds);
    }

    return NULL;
}

void leds_init(void)
{
    //We launch a new thread to interact with the LED drivers
    static char led_thread_stack[THREAD_STACKSIZE_DEFAULT];
    led_pid = thread_create(led_thread_stack, THREAD_STACKSIZE_DEFAULT,
                            THREAD_PRIORITY_MAIN - 1, 0, led_thread, NULL, "led_thread");
}
