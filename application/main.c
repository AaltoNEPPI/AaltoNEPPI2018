/*
 * This is a test file for AaltoNeppi2018 LED API.
 *
 * Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is open source
 */
#include <stdio.h>
#include <string.h>

//Doesn't seem to work here?
//#define ENABLE_DEBUG (1)
//#include "debug.h"
#include "thread.h"
#include "leds.h"
#include "xtimer.h"
#include "ble_neppi.h"
#include "periph/gpio.h"

#define MAIN_RCV_QUEUE_SIZE  (8)
#define BLE_UUID_CONTROLS_CHARACTERISTIC                 0xBBD0
#define BLE_UUID_ENERGY_CHARACTERISTIC                   0xBBCF

color_hsv_t parse_message_to_hsv(uint16_t message_content)
{
    float h = message_content;
    printf("Message content was: %d\n", message_content);
    if (h > 360.0) {
        h = 360.0;
    }
    if (h < 0.0) {
        h= 0.0;
    }
    color_hsv_t hsv_color = {
    .h = h,
    .s = 1.0,
    .v = 0.5,
    };
    return hsv_color;
}

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
    ble_neppi_init(main_pid);
    char_descr_t desc;
    //TODO test with more than 6 characteristics.
    //TODO prevent same UUID from being used twice.
    ble_neppi_add_char(BLE_UUID_CONTROLS_CHARACTERISTIC,desc,13);
    ble_neppi_add_char(BLE_UUID_ENERGY_CHARACTERISTIC,desc,15);
    ble_neppi_start();

    msg_t main_message;
    uint8_t b = 0;
    uint8_t state = 0;
    for (;;) {
        if (msg_try_receive(&main_message) == 1) {
            if (main_message.type == CHANGE_COLOR) {
                DEBUG("Message value was: %d", main_message.content.value);
                color_hsv_t hsv_color = parse_message_to_hsv(main_message.content.value);
                color_hsv2rgb(&hsv_color,&(led_color.color));
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
            ble_neppi_update_char(BLE_UUID_CONTROLS_CHARACTERISTIC, state);
            DEBUG("New button state: %d\n", state);
        }
        b = led_color.color.b;
        b = b + 1;
        if (b == 255) {
            b = 0;
        }
        led_color.color.b = b;
        leds_set_color(led_color);
        xtimer_sleep(1);
    }
    /* NOTREACHED */
}
