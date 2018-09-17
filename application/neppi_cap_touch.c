/*
 * Capacitive touch for AaltoNeppi2018.
 *
 * Author: Pekka Nikander <pekka.nikander@iki.fi>
 *
 * This code is placed in public domain.
 */

#include <stdio.h>
#include <string.h>

#define ENABLE_DEBUG (1)

#include "debug.h"

#include "led.h"
#include "xtimer.h"

#include "periph/gpio.h"
#include "periph/adc.h"

#include "../drivers/cap_touch/cap_touch.h" // XXX
#include "../drivers/cap_touch/include/cap_touch_params.h" // XXX

#include "neppi_cap_touch.h"

cap_touch_t dev;

static void cb(cap_touch_button_state_t state, uint16_t data, void *arg) {
    kernel_pid_t target_pid = (kernel_pid_t)(int)arg;
    
    uint16_t type;
    switch (state) {
    case ON:  type = MESSAGE_BUTTON_ON;  break;
    case OFF: type = MESSAGE_BUTTON_OFF; break;
    }
    msg_t m = {
        .type = type,
        .content = { .value = data },
    };
    msg_try_send(&m, target_pid);
}


void neppi_cap_touch_init(void)
{
    int r = cap_touch_init(&dev, cap_touch_params);
    if (r < 0)
	core_panic(PANIC_GENERAL_ERROR, "XXX");

    cap_touch_calibrate(&dev);
}

void neppi_cap_touch_start(kernel_pid_t target_pid) {
    cap_touch_start(&dev, cb, (void*)(int)target_pid,
		    CAP_TOUCH_START_FLAG_REPORT_ALL /* XXX */);
}

#if 1
void neppi_cap_touch_reset(void)
{
    cap_touch_reset(&dev);
}
#endif
