/*
 * This is a test file for AaltoNeppi2018 BLE API. It changes LED color
 * and intensity according to messages it receives from a BLE client.
 *
 * Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code here is open source
 */
#include <stdio.h>
#include <string.h>

//#define ENABLE_DEBUG (1)
//#include "debug.h"
#include "thread.h"
#include "leds.h"
#include "xtimer.h"
#include "ble_neppi.h"
#include "periph/gpio.h"

#define MAIN_RCV_QUEUE_SIZE                 (8)
#define BLE_UUID_CONTROLS_CHARACTERISTIC    0xABBB
#define BLE_UUID_H                          0xBBD0
#define BLE_UUID_V                          0xBBCF

/**
 * Helper function to change RIOT messages into hsv color.
 */
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
/**
 * Simple color cycle function.
 */
color_rgb_t cycle_colors(color_rgb_t * rgb_color)
{
    color_hsv_t hsv;
    color_rgb_t new_rgb;

    color_rgb2hsv(rgb_color, &hsv);
    hsv.h += 1.0;
    if (hsv.h > 360.0) {
        hsv.h = 0;
    }
    color_hsv2rgb(&hsv, &new_rgb);
    return new_rgb;
}

static msg_t main_rcv_queue[MAIN_RCV_QUEUE_SIZE];

int main(int ac, char **av)
{
    // Initialize buttons
    gpio_init(BTN0_PIN, BTN0_MODE);
    gpio_init(BTN1_PIN, BTN1_MODE);
    gpio_init(BTN2_PIN, BTN2_MODE);
    gpio_init(BTN3_PIN, BTN3_MODE);
    // Initialize LED API. This starts a new thread.
    leds_init();
    LED0_TOGGLE;
    color_rgba_t led_color = {
        .color = { .r = 255, .g = 0, .b = 0, },
        .alpha = 100,
    };
    // Initialize message queue.
    msg_init_queue(main_rcv_queue, MAIN_RCV_QUEUE_SIZE);
    // Change the color at start to red.
    leds_set_color(led_color);
    // Initialize BLE. The BLE thread needs main PID to know where to send
    // messages.
    kernel_pid_t main_pid = thread_getpid();
    ble_neppi_init(main_pid);

    // Add characteristics. We use one to send, two to receive.
    //TODO test with more than 6 characteristics.
    char_descr_t desc;
    ble_neppi_add_char(BLE_UUID_CONTROLS_CHARACTERISTIC, desc, 0);
    ble_neppi_add_char(BLE_UUID_H,desc,13);
    ble_neppi_add_char(BLE_UUID_V,desc,15);
    // Start the ble execution.
    ble_neppi_start();

    // Switch to main execution loop.
    msg_t main_message;
    uint8_t state = 0;
    for (;;) {
        // Check if we have received any color or intensity values from client.
        if (msg_try_receive(&main_message) == 1) {
            if (main_message.type == BLE_UUID_H) {
                DEBUG("Message value was: %d", main_message.content.value);
                color_hsv_t hsv_color = parse_message_to_hsv(main_message.content.value);
                //This function alters led_color.color directly. See RIOT color.c
                color_hsv2rgb(&hsv_color,&(led_color.color));
            }
            if (main_message.type == BLE_UUID_V) {
                DEBUG("Intensity message was: %d", main_message.content.value);
                led_color.alpha = (uint8_t)(main_message.content.value);
            }
        }
        // Check if buttons have been pressed.
        uint8_t new_state = 0;
        new_state |= (!gpio_read(BTN0_PIN)) << 0;
        new_state |= (!gpio_read(BTN1_PIN)) << 1;
        new_state |= (!gpio_read(BTN2_PIN)) << 2;
        new_state |= (!gpio_read(BTN3_PIN)) << 3;

        if (new_state != state) {
            state = new_state;
            ble_neppi_update_char(BLE_UUID_CONTROLS_CHARACTERISTIC, state);
            DEBUG("New button state: %d\n", state);
        }
        // Cycle through colors, so we know we are still alive.
        led_color.color = cycle_colors(&(led_color.color));
        leds_set_color(led_color);
        xtimer_sleep(1);
    }
    /* NOTREACHED */
    return 0;
}
