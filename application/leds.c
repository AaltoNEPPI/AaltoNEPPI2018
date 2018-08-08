/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is public domain.
 */
#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "leds.h"

void led_set_color(color_rgba_t *leds, kernel_pid_t led_pid)
{
    /*
     * The apa102 driver wants a color_rgba_t input. Problem is,
     * even though we have it, RIOT messages are uint32_t, so we have
     * to pack the colors into a single value, only to break them up again.
     */
    msg_t m;
    for (uint16_t i = 0; i < (uint16_t)APA102_PARAM_LED_NUMOF; i++)
    {
        //reset the message contents
        memset(&m,0,sizeof(msg_t));
        //message type reveals which led the colors belong to
        //if we spec that all LEDS always show same color, things simplify quite
        //a bit
        m.type = i;
        //pack the values and send them off. Maybe you could cast a color_rgba_t into
        //uint32_t and send it, but I'm not sure about the compiler padding.
        m.content.value |= ((uint32_t)leds[i].alpha << 24);
        m.content.value |= ((uint32_t)leds[i].color.b << 16);
        m.content.value |= ((uint32_t)leds[i].color.g << 8);
        m.content.value |= ((uint32_t)leds[i].color.r);
        printf("%d\n",m.content.value);
        msg_try_send(&m, led_pid);
    }
}


void *led_thread(void *arg)
{
    (void) arg;
    //initialize the message queue
    //TODO, variable led amounts. Can too many crash us?
    msg_t rcv_queue[APA102_PARAM_LED_NUMOF];
    msg_init_queue(rcv_queue,APA102_PARAM_LED_NUMOF);
    //initialize apa102
    apa102_t dev;
    apa102_init(&dev, &apa102_params[0]);

    color_rgba_t leds[APA102_PARAM_LED_NUMOF];
    int message_count = 0;
    msg_t m;
    while (1)
    {
        //We now wait to receive a 32-bit package that we need to parse back into colors.
        msg_receive(&m);
        uint16_t index = m.type;
        //red mask is 0x000000ff
        leds[index].color.r = m.content.value & 0x000000ff;
        //green mask is 0x0000ff00
        leds[index].color.g = ((m.content.value & 0x0000ff00) >> 8);
        //blue mask is 0x00ff0000
        leds[index].color.b = ((m.content.value & 0x00ff0000) >> 16);
        //alpha mask is 0xff000000
        leds[index].alpha = ((m.content.value & 0xff000000) >> 24);
        printf("%d\n",leds[index].color.r);
        printf("%d\n",leds[index].color.g);
        printf("%d\n",leds[index].color.b);
        printf("%d\n",leds[index].alpha);
        //what we do here is wait until we have (maybe) all LEDS full. This simplifies
        //if they are all the same color.
        message_count = message_count + 1;
        if (message_count >= APA102_PARAM_LED_NUMOF)
        {
            apa102_load_rgba(&dev,leds);
            message_count = 0;
        }
    }

    return NULL;
}

kernel_pid_t leds_init(char *led_thread_stack)
{
    //where can this be allocated better?
    //char led_thread_stack[THREAD_STACKSIZE_MAIN];
    kernel_pid_t led_pid = thread_create(led_thread_stack, THREAD_STACKSIZE_DEFAULT,
                            THREAD_PRIORITY_MAIN - 1, 0, led_thread, NULL, "led_thread");
    return led_pid;
}
