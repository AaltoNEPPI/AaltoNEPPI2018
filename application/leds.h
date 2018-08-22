/**
 * Made by Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is public domain here.
 */
#ifndef LEDDRIVERH
#define LEDDRIVERH

#include "thread.h"
#include "xtimer.h"
#include "color.h"

#define MESSAGE_COLOR_NEW           122
#define MESSAGE_COLOR_SET_HOLD      123
#define MESSAGE_COLOR_SET_CYCLE     124
#define MESSAGE_COLOR_ACTIVE        125
#define MESSAGE_COLOR_SLEEP         126
#define MESSAGE_COLOR_NEW_H         127
#define MESSAGE_COLOR_NEW_V         128

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Changes the color of the LEDS to match led_color.
 */
void leds_set_color(color_rgba_t led_color);

/**
 *Initializes the led thread and the underlying drivers.
 */
void leds_init(kernel_pid_t main_pid);

/**
 * API function to put LEDs into sleep.
 */
void leds_sleep(void);

/**
 * API function to put LEDs into active mode.
 */
void leds_active(void);

/**
 * API function to make LEDs cycle colors.
 */
void leds_cycle(void);

/**
 * API function to make LEDs stop cycling.
 */
void leds_hold(void);

#ifdef __cplusplus
}
#endif

#endif //Include guard
