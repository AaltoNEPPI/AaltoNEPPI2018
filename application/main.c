/*
 * This is the main file for AaltoNeppi2018.
 *
 * Made by Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is placed in public domain.
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
#include "mpu_neppi.h"
//#include "periph/adc.h"

#define MAIN_RCV_QUEUE_SIZE                 (8)
#define BLE_UUID_CONTROLS_CHARACTERISTIC    0xABBB
#define BLE_UUID_H                          0xBBD0
#define BLE_UUID_V                          0xBBCF
#define BLE_UUID_CYCLING                    0xAACF
#define BLE_UUID_ACTIVE                     0xCCCF


/**
 * Helper function to change RIOT messages into hsv color.
 */
color_hsv_t parse_message_to_hsv(uint32_t message_content)
{
    float h = message_content;
    printf("Message content was: %u\n", message_content);
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

//Work in progress adc. Currently freezes threads.
/*
NORETURN static void *adc_thread(void *arg)
{
    (void)arg;
    for (;;) {
        xtimer_sleep(10);
        int adc_val = adc_sample(NRF52_AIN5, ADC_RES_12BIT);
        printf("Sample was: %d\n", adc_val);
    }
}
*/
static msg_t main_rcv_queue[MAIN_RCV_QUEUE_SIZE];

int main(int ac, char **av)
{
    LED0_OFF; // On by default on this PCB

    msg_init_queue(main_rcv_queue, MAIN_RCV_QUEUE_SIZE);
    kernel_pid_t main_pid = thread_getpid();
    xtimer_sleep(1);
    // Initialize buttons
    gpio_init(BTN0_PIN, BTN0_MODE);
    /* XXX ADC code
    int failure = adc_init(NRF52_AIN5);
    printf("Did we fail? %d\n", failure);
    xtimer_sleep(1);
    */
    // Initialize BLE
    kernel_pid_t ble_pid = ble_neppi_init(main_pid);
    xtimer_sleep(1);
    // Define descriptions for BLE characteristics. Currently only length.
    char_descr_t mpu_desc = {
        .char_len = 18,
    };
    char_descr_t h_desc = {
        .char_len = 2,
    };
    char_descr_t generic_desc = {
        .char_len = 1,
    };
    // MPU data. Should be at least 18 bytes.
    ble_neppi_add_char(BLE_UUID_CONTROLS_CHARACTERISTIC, mpu_desc, 0);
    // Color hue control. Values 0-360
    ble_neppi_add_char(BLE_UUID_H, h_desc, 0);
    // Color intensity control. Values 0-255
    ble_neppi_add_char(BLE_UUID_V, generic_desc, 0);
    // LED cycle clientside status control. 0-1
    ble_neppi_add_char(BLE_UUID_CYCLING, generic_desc, 1);
    // LED sleep clientside status control. 0-1
    ble_neppi_add_char(BLE_UUID_ACTIVE, generic_desc, 0);

    // Initialize LEDs
    leds_init(main_pid);
    // Initialize the MPU9250.
    mpu_neppi_init(main_pid, ble_pid, BLE_UUID_CONTROLS_CHARACTERISTIC);
    // Start the ble execution.
    ble_neppi_start();
    // Start the MPU execution.
    mpu_neppi_start();

    // Put LED color to red.
    color_rgba_t led_color = {
        .color = { .r = 255, .g = 0, .b = 0, },
        .alpha = 100,
    };
    leds_set_color(led_color);
    /* XXX ADC code
    static char adc_thread_stack[(THREAD_STACKSIZE_DEFAULT)];
    thread_create(adc_thread_stack, sizeof(adc_thread_stack),
                   THREAD_PRIORITY_MAIN + 1, 0,
                   adc_thread, NULL, "ADC");
    */
    // Make LEDs cycle
    leds_cycle();
    // Switch to main execution loop.
    msg_t main_message;
    for (;;) {
        msg_receive(&main_message);
        switch (main_message.type) {
            case BLE_UUID_H:
                // Client sent a new hue
                DEBUG("Message value was: %d",main_message.content.value);
                color_hsv_t hsv_color = parse_message_to_hsv(main_message.content.value);
                //This function alters led_color.color directly. See RIOT color.c$
                color_hsv2rgb(&hsv_color,&(led_color.color));
                leds_set_color(led_color);
                break;
            case BLE_UUID_V:
                // Client sent a new intensity
                DEBUG("Intensity message was: %d", main_message.content.value);
                led_color.alpha = (uint8_t)(main_message.content.value);
                leds_set_color(led_color);
                break;
            case BLE_UUID_CYCLING:
                // Client sent a command concerning color cycling
                if (main_message.content.value == 1) {
                    leds_cycle();
                }
                if (main_message.content.value == 0) {
                    leds_hold();
                }
                break;
            case BLE_UUID_ACTIVE:
                // Client sent a command concerning led status
                if (main_message.content.value == 1) {
                    leds_active();
                }
                if (main_message.content.value == 0) {
                    leds_sleep();
                }
                break;
            case MESSAGE_COLOR_NEW_H:
                // LEDs just changed color, send to Client
                ble_neppi_update_char(BLE_UUID_H, main_message.content.value);
                break;
            case MESSAGE_COLOR_NEW_V:
                // LEDs just changed intensity, send to Client
                ble_neppi_update_char(BLE_UUID_V, main_message.content.value);
                break;
            case MESSAGE_MPU_ACTIVE:
                // MPU detected motion, set LED status and notify Client
                leds_active();
                leds_hold();
                ble_neppi_update_char(BLE_UUID_ACTIVE, 1);
                break;
            case MESSAGE_MPU_SLEEP:
                // MPU detected stillness, set LED status and notify Client
                leds_sleep();
                leds_cycle();
                ble_neppi_update_char(BLE_UUID_ACTIVE, 0);
            default:
                break;
        }
    }
    /* NOTREACHED */
    return 0;
}
