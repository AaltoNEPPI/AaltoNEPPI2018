/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is public domain.
 */
#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "leds.h"

static kernel_pid_t led_pid;

void leds_set_color(color_rgba_t led_color){
    /*
     * The apa102 driver wants a color_rgba_t input. Problem is,
     * even though we have it, RIOT messages are uint32_t, so we have
     * cast the data type before sending.
     */
    msg_t m;
    //Message needs to be set to 0. Otherwise garbage appears.
    memset(&m, 0, sizeof(msg_t));
    //Assert that our message can fit the leds. It should.
    static_assert(sizeof(m.content.value) >= sizeof(led_color));
    //Place the color into the message.
    *((color_rgba_t *)&m.content.value) = led_color;
    //printf("%d\n",m.content.value);
    //try_send because we want this function to be non-blocking
    msg_try_send(&m, led_pid);
}


static void *led_thread(void *arg){
    (void) arg;
    //initialize the message queue
    msg_t rcv_queue[4];
    msg_init_queue(rcv_queue,4);
    //initialize apa102.
    static apa102_t dev;
    apa102_init(&dev, &apa102_params[0]);

    color_rgba_t led_color;
    static color_rgba_t leds[APA102_PARAM_LED_NUMOF];
    msg_t m;
    for (;;){
        //We now wait to receive a 32-bit package that we need to cast
        //back into colors.
        msg_receive(&m);
        led_color = *((color_rgba_t *)&m.content.value);
        //apa102_load_rgba wants an array of colors for each leds, so
        //we copy the same color to each index
        for (int i = 0; i < APA102_PARAM_LED_NUMOF; i++){
            leds[i] = led_color;
        }
        //Fire it off! The drivers will do the rest
        apa102_load_rgba(&dev,leds);
    }

    /* NOTREACHED */

    return NULL;
}

void leds_init(void){
    //We launch a new thread to interact with the LED drivers
    static char led_thread_stack[THREAD_STACKSIZE_DEFAULT];
    led_pid = thread_create(led_thread_stack, sizeof(led_thread_stack),
                            THREAD_PRIORITY_MAIN - 1, 0, led_thread, NULL, "led_thread");
}
