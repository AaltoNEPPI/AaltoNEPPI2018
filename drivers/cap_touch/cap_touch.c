/*
 * Copyright (C) 2018 Aalto University
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_cap_touch
 * @{
 *
 * @file
 * @brief       Simple capacitive touch button driver implementation
 *
 * @author      Pekka Nikander <pekka.nikander@iki.fi>
 *
 * @}
 */

#include <string.h>
#include <assert.h>
#include <limits.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "cap_touch.h"

#include "thread.h"
#include "mutex.h"
#include "xtimer.h"

uint32_t cap_touch_sample(const cap_touch_t *dev, adc_t line) {
    static mutex_t lock = MUTEX_INIT;
    int sample_sum = 0;

    // Check that the int is large enough for averaging
    static_assert(sizeof(sample_sum) > 2);

    mutex_lock(&lock);
    for (int i = 0; i < dev->sample_rounds; i++) {
	// Ground the pin for a while
	gpio_init(dev->sense_pin, GPIO_OUT);
	gpio_clear(dev->sense_pin);
	xtimer_usleep(dev->grounding_time);
	// Convert the pin back to an input
	gpio_init(dev->sense_pin, GPIO_IN);
	// Sample the pin
	sample_sum += adc_sample(line, dev->res);
    }
    mutex_unlock(&lock);
    // Return the average of the samples
    return sample_sum/(dev->sample_rounds);
}

struct args {
    const cap_touch_t *dev;
    adc_t line;
    cap_touch_cb_t cb;
    void *cb_arg;
};

NORETURN static void *cap_touch_thread(void *arg)
{
    const struct args *args = arg;
    const cap_touch_t *dev = args->dev;

    adc_init(args->line);

    DEBUG("cap_touch: calibrating line %d for %d rounds...\n",
	  args->line, dev->calib_rounds);
    int min_sample = INT_MAX;
    int max_sample = 0;
    for (int i = 0; i < dev->calib_rounds; i++) {
	int sample = cap_touch_sample(dev, args->line);

	if (sample < min_sample) min_sample = sample;
	if (sample > max_sample) max_sample = sample;
    }
    DEBUG("cap_touch: calibrated: %d-%d.\n", min_sample, max_sample);

    enum state {
	OFF, ON
    } state = OFF;

    int hyst_count = 0;

    for (;;) {
	int sample = cap_touch_sample(dev, args->line);

	switch (state) {
	case OFF:
	    if (sample < min_sample || sample > max_sample) {
		if (hyst_count++ < dev->hysteresis)
		    continue;
		state = ON;
		hyst_count = 0;
		args->cb(args->cb_arg, sample >> 8 /*XXX*/);
	    }
	    break;
	case ON:
	    if (sample > min_sample && sample < max_sample) {
		if (hyst_count++ < dev->hysteresis)
		    continue;
		state = OFF;
		hyst_count = 0;
		args->cb(args->cb_arg, 0);
	    }
	}
    }
}

#ifndef CPU_MODEL_NRF52832XXAA
#error "Please define pin2line for your CPU model"
#endif

static int pin2line(gpio_t pin) {
    switch (pin) {
    case GPIO_PIN(0,  2): return ADC_LINE(0);
    case GPIO_PIN(0,  3): return ADC_LINE(1);
    case GPIO_PIN(0,  4): return ADC_LINE(2);
    case GPIO_PIN(0,  5): return ADC_LINE(3);
    case GPIO_PIN(0, 28): return ADC_LINE(4);
    case GPIO_PIN(0, 29): return ADC_LINE(5);
    case GPIO_PIN(0, 30): return ADC_LINE(6);
    case GPIO_PIN(0, 31): return ADC_LINE(7);
    default:              return -1;
    }
}

int cap_touch_init(cap_touch_t *dev, const cap_touch_params_t *params)
{
    assert(dev && params);

    memcpy(dev, params, sizeof(cap_touch_params_t));
    const int line = pin2line(dev->sense_pin);
    if (line < 0)
	return -1;
    if (dev->sample_rounds < 1)
	return -2;
    if (dev->calib_rounds < 1)
	return -3;
    if (dev->grounding_time < 1)
	return -4;
    return 0;
}

void cap_touch_start(cap_touch_t *dev, cap_touch_cb_t cb, void *arg) {
    static char thread_stack[THREAD_STACKSIZE_DEFAULT*2/*XXX for now, due to SD*/];
    static struct args args; // XXX: Does this need to be static?

    args.dev    = dev;
    args.line   = pin2line(dev->sense_pin);
    args.cb     = cb;
    args.cb_arg = arg;

    thread_create(thread_stack, sizeof(thread_stack),
		  THREAD_PRIORITY_MAIN + 1, THREAD_CREATE_WOUT_YIELD,
		  cap_touch_thread, &args, "CT");
    DEBUG("cap_touch: thread created\n");
}

