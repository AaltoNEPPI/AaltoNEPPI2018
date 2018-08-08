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

//Takes in an array of colors with alpha, and sends a message to the led
//thread according to the pid
void led_set_color(color_rgba_t *leds, kernel_pid_t led_pid);

//Initializes the led thread
kernel_pid_t leds_init(char *led_thread_stack);

#ifdef __cplusplus
}
#endif

#endif //Include guard
