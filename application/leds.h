/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is public domain here.
 */
#ifndef LEDDRIVERH
#define LEDDRIVERH

#include "thread.h"
#include "xtimer.h"
#include "color.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Changes the color of the LEDS to match led_color.
 */
void leds_set_color(color_rgba_t led_color);

/*
 *Initializes the led thread and the underlying drivers.
 */
void leds_init(void);

#ifdef __cplusplus
}
#endif

#endif //Include guard
