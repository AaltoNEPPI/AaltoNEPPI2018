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
#define MESSAGE_COLOR_INTENSITY     125
#define MESSAGE_COLOR_NEW_HUE       127
#define MESSAGE_COLOR_NEW_VALUE     128
#define MESSAGE_COLOR_SET_BLINK     130

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Changes the color of the LEDS to match led_color.
 */
void leds_set_color(const color_hsv_t *led_color);

/**
 *Initializes the led thread and the underlying drivers.
 */
void leds_init(kernel_pid_t main_pid);

/**
 * API function to put LEDs into active mode.
 */
void leds_set_intensity(float intensity);

/**
 * API function to make LEDs cycle colors.
 */
void leds_cycle(void);

/**
 * API function to make LEDs stop cycling.
 */
void leds_hold(void);

/**
 * Make the LEDs to blink, indicating low battery.
 */
void leds_blink(void);

/**
 * Make the LEDs completely off.
 */
void leds_off(void);

#ifdef __cplusplus
}
#endif

#endif //Include guard
