/*
 * Capacitive touch for AaltoNeppi2018.
 *
 * Author: Pekka Nikander <pekka.nikander@iki.fi>
 *
 * This code is placed in public domain.
 */

#define MESSAGE_BUTTON_OFF 1000
#define MESSAGE_BUTTON_ON  1001

void neppi_cap_touch_init(void);
void neppi_cap_touch_start(kernel_pid_t target_pid);
