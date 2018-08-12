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
#include "neppible.h"

#define MAIN_RCV_QUEUE_SIZE  (8)
#define BLE_UUID_CONTROLS_CHARACTERISTIC                 0xBBD0
#define BLE_UUID_ENERGY_CHARACTERISTIC                   0xBBCF

static msg_t main_rcv_queue[MAIN_RCV_QUEUE_SIZE];

int main(int ac, char **av)
{
    // Initialize buttons
    gpio_init(BTN0_PIN, BTN0_MODE);
    gpio_init(BTN1_PIN, BTN1_MODE);
    gpio_init(BTN2_PIN, BTN2_MODE);
    gpio_init(BTN3_PIN, BTN3_MODE);
    //Initialize LED API. This starts a new thread.
    leds_init();
    LED0_TOGGLE;
    color_rgba_t led_color = {
        .color = { .r = 255, .g = 0, .b = 0, },
        .alpha = 100,
    };
    //Initialize message queue.
    msg_init_queue(main_rcv_queue, MAIN_RCV_QUEUE_SIZE);
    //Change the color at start to red.
    leds_set_color(led_color);
    kernel_pid_t main_pid = thread_getpid();
    neppible_init(main_pid);
    char_descr_t desc;
    //TODO test with more than 6 characteristics.
    //TODO prevent same UUID from being used twice.
    neppible_add_char(BLE_UUID_CONTROLS_CHARACTERISTIC,desc,13);
    neppible_add_char(BLE_UUID_ENERGY_CHARACTERISTIC,desc,15);
    neppible_start();

    msg_t main_message;
    uint8_t b = 0;
    uint8_t g = 0;
    uint8_t state = 0;
    for (;;) {
        if (msg_try_receive(&main_message) == 1) {
            if (main_message.type == CHANGE_COLOR) {
                DEBUG("Message value was: %d", main_message.content.value);
                g = (uint8_t)main_message.content.value;
            }
        }
        uint8_t new_state = 0;

        new_state |= (!gpio_read(BTN0_PIN)) << 0;
        new_state |= (!gpio_read(BTN1_PIN)) << 1;
        new_state |= (!gpio_read(BTN2_PIN)) << 2;
        new_state |= (!gpio_read(BTN3_PIN)) << 3;

        if (new_state != state) {
            state = new_state;
            //TODO Magic number right now
            //main_message.type = 1;
            //main_message.content.value = state;
            //msg_send(&main_message, ble_thread_pid);
            neppible_update_char(BLE_UUID_CONTROLS_CHARACTERISTIC, state);
            DEBUG("New button state: %d\n", state);
        }
        b = b + 10;
        if (b > 240) {
            b = 0;
        }
        led_color.color.b = b;
        led_color.color.g = g;
        leds_set_color(led_color);
        xtimer_sleep(1);
    }
    /* NOTREACHED */
}
