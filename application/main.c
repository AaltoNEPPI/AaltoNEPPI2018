/*
 * This is a test file for AaltoNeppi2018 LED API.
 *
 * Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
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
    color_rgba_t led_color = {
        .color = { .r = 255, .g = 0, .b = 0, },
        .alpha = 100,
    };
    //Change the color at start to red.
    leds_set_color(led_color);
    //Slowly increase blueness
    uint8_t b = 0;
    for (;;) {
        b = b + 10;
        if (b > 240) {
            b = 0;
        }
        led_color.color.b = b;
        leds_set_color(led_color);
        xtimer_sleep(1);
    }
    /* NOTREACHED */
}
