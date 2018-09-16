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
#include "neppi_cap_touch.h"
#include "neppi_shell.h"

/**
 * Brightness during initialisation
 */
#define LED_HUE_INIT1              (120.00f) // Green
#define LED_HUE_INIT2              (240.00f) // Blue
#define LED_INTENSITY_INIT           (0.02f) // 2%
#define LED_SATURATION_INIT          (1.00f) // 100%

/**
 * Brightness in "OFF" state
 */
#define LED_INTENSITY_OFF            (0.01f) // 1%

/**
 * Brightness of the device when it is sleeping. Between 0 and 255.
 */
#define LED_HUE_SLEEP                 (0.00f) // Red
#define LED_INTENSITY_SLEEP           (0.05f) // 5%

/**
 * Brightness of the device when it is active.
 */
#define LED_INTENSITY_ACTIVE          (0.40f) // 50%

/**
 * Brightness of the device when the button is pressed.
 */
#define LED_INTENSITY_PAINTING         (1.00) // 100%

/**
 * XXX
 */
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

int main(int ac, char **av)
{
    DEBUG("Entering main function.\n");
    LED0_OFF; // On by default on this PCB

    static msg_t main_rcv_queue[MAIN_RCV_QUEUE_SIZE];
    msg_init_queue(main_rcv_queue, MAIN_RCV_QUEUE_SIZE);

    kernel_pid_t main_pid = thread_getpid();
    xtimer_usleep(100);

    // Initialize BLE
    kernel_pid_t ble_pid = ble_neppi_init(
        main_pid, &NEPPI_BLE_UUID_BASE, NEPPI_BLE_UUID_SERVICE);

    // Add characteristics. We use one to send mpu data, two to send and
    // receive color data. The last 2 are for controlling color cycling
    // and sleep status. Note that cycling and sleep is also controlled
    // by the MPU.
    //TODO test with more than 6 characteristics.
    const char_descr_t mpu_desc   = { .char_len = 18 /* XXX */, };
    const char_descr_t hue_desc   = { .char_len =  2 /* XXX */, };
    const char_descr_t byte_desc  = { .char_len =  1 /* XXX */, };
    const char_descr_t state_desc = { .char_len =  3 /* XXX */, };

    ble_neppi_add_char(NEPPI_BLE_UUID_CONTROLS,      mpu_desc, 0);
    ble_neppi_add_char(NEPPI_BLE_UUID_COLOR_HUE,     hue_desc, 0);
    ble_neppi_add_char(NEPPI_BLE_UUID_COLOR_VALUE,   byte_desc, 0);
    ble_neppi_add_char(NEPPI_BLE_UUID_COLOR_CYCLING, byte_desc, 1);
    ble_neppi_add_char(NEPPI_BLE_UUID_STATE,         state_desc, 0);

    // Initialize LEDs
    leds_init(main_pid);
    // Initialize the MPU9250.
    mpu_neppi_init(main_pid, ble_pid, NEPPI_BLE_UUID_CONTROLS);

    // low green
    static const color_hsv_t dim_green = {
        .h = LED_HUE_INIT1,
        .s = LED_SATURATION_INIT,
        .v = LED_INTENSITY_INIT,
    };
    leds_set_color(&dim_green);

    // Wait for five seconds for the user to set the device
    // on the table
    puts("Please place the device on table for calibration.");
    xtimer_sleep(5);

    puts("Starting calibration.");
    // Start the ble execution.
    ble_neppi_start();

    // low blue glow during calibration
    static const color_hsv_t dim_blue = {
        .h = LED_HUE_INIT2,
        .s = LED_SATURATION_INIT,
        .v = LED_INTENSITY_INIT,
    };
    leds_set_color(&dim_blue);

    // Initialise and calibrate capacitive touch
    neppi_cap_touch_init();

    // Start the MPU execution.
    mpu_neppi_start();

    // Put LED color to red.
    color_hsv_t led_color = {
        .h = LED_HUE_SLEEP,
        .s = LED_SATURATION_INIT,
        .v = LED_INTENSITY_SLEEP,
    };
    leds_set_color(&led_color);

    // Make LEDs cycle
    leds_cycle();

    // Start the capacitive touch
    DEBUG("main: initialising capacitive touch...\n");
    neppi_cap_touch_start(main_pid);
    DEBUG("main: initialising capacitive touch...done.\n");

    // Start the shell
    neppi_shell_start();

    DEBUG("Entering main loop.\n");

    enum { INIT, OFF, SLEEP, ACTIVE, PAINTING } state = OFF, prev_state = INIT;

    for (;;) {
        msg_t main_message;

        msg_receive(&main_message);

        int value = main_message.content.value;

        switch (main_message.type) {
        case NEPPI_BLE_UUID_COLOR_HUE:
            // Client sent a new hue
            DEBUG("Set hue: %d\n", value);
            led_color.h = value;
            leds_set_color(&led_color);
            break;
        case NEPPI_BLE_UUID_COLOR_VALUE:
            // Client sent a new intensity
            DEBUG("Set intensity: %d\n", value);
            led_color.v = (uint8_t)value;
            leds_set_color(&led_color);
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
            state = value;
            if (value) {
                leds_set_intensity(LED_INTENSITY_ACTIVE);
            } else {
                leds_set_intensity(LED_INTENSITY_SLEEP);
            }
            break;

        case MESSAGE_COLOR_NEW_HUE:
            ble_neppi_update_char(NEPPI_BLE_UUID_COLOR_HUE, value);
            break;
        case MESSAGE_COLOR_NEW_VALUE:
            ble_neppi_update_char(NEPPI_BLE_UUID_COLOR_VALUE, value);
            break;

        case MESSAGE_MPU_SLEEP:
            state = SLEEP;
            break;
        case MESSAGE_MPU_ACTIVE:
            state = ACTIVE;
            break;
        case MESSAGE_BUTTON_OFF:
            switch (state) {
            case INIT:     state = OFF;    break;
            case OFF:      state = OFF;    break;
            case SLEEP:    state = SLEEP;  break;
            case ACTIVE:   state = ACTIVE; break;
            case PAINTING: state = ACTIVE; break;
            }
#if 1
	    // XXX: Debugging
	    ble_neppi_update_char(NEPPI_BLE_UUID_STATE, state | value << 8);
#endif
            break;
        case MESSAGE_BUTTON_ON:
            switch (state) {
            case INIT:     state = OFF;      break;
            case OFF:      state = SLEEP;    break;
            case SLEEP:    state = SLEEP;    break;
            case ACTIVE:   state = PAINTING; break;
            case PAINTING: state = PAINTING; break;
            }
#if 1
	    // XXX: Debugging
	    ble_neppi_update_char(NEPPI_BLE_UUID_STATE, state | value << 8);
#endif
            break;
        default:
            DEBUG("Unknown message\n");
            break;
        }

        if (state != prev_state) {
            prev_state = state;
            LED0_OFF;
            switch (state) {
            case OFF:
                DEBUG("Off:        %d\n", value);
                leds_set_intensity(LED_INTENSITY_OFF);
                leds_hold();
                ble_neppi_update_char(NEPPI_BLE_UUID_STATE, OFF      | value << 8);
                break;
            case SLEEP:
                DEBUG("Sleep:      %d\n", value);
                leds_set_intensity(LED_INTENSITY_SLEEP);
                leds_cycle();
                ble_neppi_update_char(NEPPI_BLE_UUID_STATE, SLEEP    | value << 8);
                break;
            case ACTIVE:
                DEBUG("Active:  %d\n", value);
                leds_set_intensity(LED_INTENSITY_ACTIVE);
                leds_hold();
                ble_neppi_update_char(NEPPI_BLE_UUID_STATE, ACTIVE   | value << 8);
                break;
            case PAINTING:
                DEBUG("Painting:  %d\n", value);
                leds_set_intensity(LED_INTENSITY_PAINTING);
                LED0_ON;
                ble_neppi_update_char(NEPPI_BLE_UUID_STATE, PAINTING | value << 8);
                break;
            default:
                core_panic(PANIC_HARD_REBOOT, "Unknown state.");
                /* NOTREACHED */
            }
        }
    }
    /* NOTREACHED */
    return 0;
}
