/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is open source
 */
#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "leds.h"
#include "color.h"
#include "xtimer.h"


int main(int ac, char **av)
{
    //This shouldn't be here, but I don't know how to transfer it from the leds_init()
    //function to the led_set_color() function without passing through main.
    //Maybe some sort of extern variable? But decleared where, how?
    kernel_pid_t led_pid;

    //Similar thing here. I can't call it in leds_init(), because won't it be freed
    //when the function returns?
    char led_thread_stack[(THREAD_STACKSIZE_DEFAULT)];
    led_pid = leds_init(led_thread_stack);
    LED2_TOGGLE;
    //Is APA102_PARAM visibility okay here? We can hide it if we always
    //light all leds with the same color. Otherwise we need to know
    //how many leds there are, and this is it.
    color_rgba_t leds[APA102_PARAM_LED_NUMOF];
    
    //Now we fill all the colors into the led array. Could be hidden in API if
    //Colors were all the same.
    for (int i = 0; i < APA102_PARAM_LED_NUMOF; i++)
    {
        leds[i].alpha = 100;
        memset(&leds[i].color, 0, sizeof(color_rgb_t));
        leds[i].color.r = 255;
        leds[i].color.g = 0;
        leds[i].color.b = 0;
    }
    LED0_TOGGLE;
    //currently only switch the color once
    led_set_color(leds, led_pid);
    LED1_TOGGLE;
    while(1)
    {
        xtimer_sleep(1);
        LED1_TOGGLE;
    }
    /*
    color_hsv_t col = { 0.0, 1.0, 0.5);
    xtimer_ticks32_t now = xtimer_now();
    while(1)
    {
        
        color.h += 1.0;
        if (color.h > 360.0)
        {
            color.h = 0.0;
        }
        
        //TODO more leds?
        for (int i = 0; i < 4; i++)
        {
            
        }
        led_set_color(&color, leds);
        xtimer_periodic_wakeup(&now, STEP);

    }
    */
    return 0;
}
