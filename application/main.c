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
    //Initialize LED API. This starts a new thread.
    leds_init();
    LED2_TOGGLE;
    color_rgba_t led_color;
    //Apparently the struct needs to be set to 0
    memset(&led_color, 0, sizeof(color_rgb_t));
    led_color.color.r = 255;
    led_color.color.g = 0;
    led_color.color.b = 0;
    led_color.alpha = 100;
    LED0_TOGGLE;
    led_set_color(led_color);
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
