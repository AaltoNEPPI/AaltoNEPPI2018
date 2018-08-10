/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is public domain here.
 */
#ifndef LEDDRIVERH
#define LEDDRIVERH

#include "thread.h"
#include "apa102.h"
#include "apa102_params.h"
#include "xtimer.h"
#include "color.h"


#ifdef __cplusplus
extern "C" {
#endif

//Takes in an array of colors with alpha
void leds_set_color(color_rgba_t led_color);

//Initializes the led thread
void leds_init(void);

#ifdef __cplusplus
}
#endif

#endif //Include guard
