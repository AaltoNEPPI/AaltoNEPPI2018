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
#include "nrf_sdh_ble.h"
#include "ble_neppi.h"
#include "periph/gpio.h"
#include "mpu_neppi.h"
//#include "periph/adc.h"

#define MAIN_RCV_QUEUE_SIZE                 (8)

/*************
 * UUIDs used for the service and characteristics
 *************/

// 128-bit base UUID
static const ble_uuid128_t NEPPI_BLE_UUID_BASE = {
    {
	0x8D, 0x19, 0x7F, 0x81,
	0x08, 0x08, 0x12, 0xE0,
	0x2B, 0x14, 0x95, 0x71,
	0x05, 0x06, 0x31, 0xB1,
    }
};

// Just a random, but recognizable value for the service
#define NEPPI_BLE_UUID_SERVICE       0xABDC

#define NEPPI_BLE_UUID_CONTROLS      0xABBB
#define NEPPI_BLE_UUID_COLOR_HUE     0xBBD0
#define NEPPI_BLE_UUID_COLOR_VALUE   0xBBCF
#define NEPPI_BLE_UUID_COLOR_CYCLING 0xAACF
#define NEPPI_BLE_UUID_STATE         0xCCCF


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
        h = 0.0;
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
int main(int ac, char **av)
{
    DEBUG("Entering main function.\n");
    LED0_OFF; // On by default on this PCB

    static msg_t main_rcv_queue[MAIN_RCV_QUEUE_SIZE];
    msg_init_queue(main_rcv_queue, MAIN_RCV_QUEUE_SIZE);

    kernel_pid_t main_pid = thread_getpid();
    xtimer_usleep(100);

#if 0
    // XXX ADC code
    int failure = adc_init(NRF52_AIN5);
    printf("Did we fail? %d\n", failure);
    xtimer_usleep(100);
#endif

    // Initialize BLE
    kernel_pid_t ble_pid = ble_neppi_init(
	main_pid, &NEPPI_BLE_UUID_BASE, NEPPI_BLE_UUID_SERVICE);

    // Add characteristics. We use one to send mpu data, two to send and
    // receive color data. The last 2 are for controlling color cycling
    // and sleep status. Note that cycling and sleep is also controlled
    // by the MPU.
    //TODO test with more than 6 characteristics.
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
    ble_neppi_add_char(NEPPI_BLE_UUID_CONTROLS, mpu_desc, 0);
    // Color hue control. Values 0-360
    ble_neppi_add_char(NEPPI_BLE_UUID_COLOR_HUE, h_desc, 0);
    // Color intensity control. Values 0-255
    ble_neppi_add_char(NEPPI_BLE_UUID_COLOR_VALUE, generic_desc, 0);
    // LED cycle clientside status control. 0-1
    ble_neppi_add_char(NEPPI_BLE_UUID_COLOR_CYCLING, generic_desc, 1);
    // LED sleep clientside status control. 0-1
    ble_neppi_add_char(NEPPI_BLE_UUID_STATE, generic_desc, 0);

    // Initialize LEDs
    leds_init(main_pid);
    // Initialize the MPU9250.
    mpu_neppi_init(main_pid, ble_pid, NEPPI_BLE_UUID_CONTROLS);
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
#if 0
    // XXX ADC code
    static char adc_thread_stack[(THREAD_STACKSIZE_DEFAULT)];
    thread_create(adc_thread_stack, sizeof(adc_thread_stack),
                   THREAD_PRIORITY_MAIN + 1, 0,
                   adc_thread, NULL, "ADC");
#endif
    // Make LEDs cycle
    leds_cycle();

    DEBUG("Entering main loop.\n");

    for (;;) {
        msg_t main_message;

        msg_receive(&main_message);

        int value = main_message.content.value;

        switch (main_message.type) {
            case NEPPI_BLE_UUID_COLOR_HUE:
                // Client sent a new hue
                DEBUG("Set hue: %d\n", value);
                color_hsv_t hsv_color = parse_message_to_hsv(value);
                //This function alters led_color.color directly. See RIOT color.c$
                color_hsv2rgb(&hsv_color, &(led_color.color));
                leds_set_color(led_color);
                break;
            case NEPPI_BLE_UUID_COLOR_VALUE:
                // Client sent a new intensity
                DEBUG("Set intensity: %d\n", value);
                led_color.alpha = (uint8_t)value;
                leds_set_color(led_color);
                break;
            case NEPPI_BLE_UUID_COLOR_CYCLING:
                // Client sent a command concerning color cycling
                DEBUG("Set cycling: %d\n", value);
                switch (value) {
                case 1: leds_cycle(); break;
                case 0: leds_hold();  break;
                }
                break;
            case NEPPI_BLE_UUID_STATE:
                // Client sent a command concerning led status
                DEBUG("Set activation: %d\n", value);
                switch (value) {
                case 1: leds_active(); break;
                case 0: leds_sleep();  break;
                }
                break;

            case MESSAGE_COLOR_NEW_H:
                // LEDs just changed color, send to Client
                DEBUG("Color hue: %d\n", value);
                ble_neppi_update_char(NEPPI_BLE_UUID_COLOR_HUE, value);
                break;
            case MESSAGE_COLOR_NEW_V:
                // LEDs just changed intensity, send to Client
                DEBUG("Intensity: %d\n", value);
                ble_neppi_update_char(NEPPI_BLE_UUID_COLOR_VALUE, value);
                break;
            case MESSAGE_MPU_ACTIVE:
                // MPU detected motion, set LED status and notify Client
                DEBUG("Active:    %d\n", value);
                leds_active();
                leds_hold();
                ble_neppi_update_char(NEPPI_BLE_UUID_STATE, 1);
                break;
            case MESSAGE_MPU_SLEEP:
                // MPU detected stillness, set LED status and notify Client
                DEBUG("Sleep:     %d\n", value);
                leds_sleep();
                leds_cycle();
                ble_neppi_update_char(NEPPI_BLE_UUID_STATE, 0);
                break;
            default:
                DEBUG("Unknown message\n");
                break;
        }
    }
    /* NOTREACHED */
    return 0;
}
