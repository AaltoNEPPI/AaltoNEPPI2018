/**
 * MPU API for AaltoNeppi2018.
 *
 * Written by Ville Hiltunen in 2018
 *
 * All code is in public domain.
 */

#include <stdlib.h>
#include "mpu_neppi.h"
#define ENABLE_DEBUG (1)
#include "debug.h"
#include "periph/gpio.h"
#include "xtimer.h"

#define MPU_RCV_QUEUE_SIZE      (8)
#define MPU_INT_PIN GPIO_PIN    (0,27)
#define MESSAGE_MPU_INTERRUPT   (885)

/**
 * MPU accelerometer and gyro sample rate. Must be between 4 and 1000
 */
#define MPU_SAMPLE_RATE         (5)

/**
 * Compass sample rate. Must be between 1 and 100
 */
#define COMPASS_SAMPLE_RATE     (5)

/**
 * Heuristic used for calculating if no significant movement is happening.
 */
#define DELTA_EPSILON           (10)

/**
 * Wake on motion threshold for movement.
 *
 * Values 0-255 are mapped to (0~1020mg)
 */
#define WOM_THRESHOLD           (50)

/**
 * Heuristic max number of times data can be the same until WoM is activated
 */
#define SHUTDOWN_COUNT_MAX      (50)

/**
 * Messaging pids.
 */
static kernel_pid_t target_pid;
static kernel_pid_t main_pid;
static kernel_pid_t mpu_thread_pid;

/**
 * Short UUID of the BLE characteristic MPU needs to update.
 */
static uint16_t uuid;

/**
 * MPU device descriptor
 */
static mpu9250_t dev;

/**
 * MPU state. 1 means active and sending data, 0 means Wake on Motion.
 */
static uint8_t mpu_active;

/**
 * Heuristic for defining how long MPU stays still before WoM activates.
 */
static int shutdown_count;

/**
 * MPU interrupt callback function.
 *
 * It sends a message to the MPU thread that an interrupt has happened.
 * It's up to the thread to find out what kind of interrupt it is.
 */
static void mpu_interrupt(void *arg) {
    msg_t m;
    m.type = MESSAGE_MPU_INTERRUPT;
    msg_try_send(&m, mpu_thread_pid);
}
/**
 * Internal helper function for initializing the mpu using driver functions.
 */
static void set_sample_rates(void)
{
    mpu9250_set_sample_rate(&dev, MPU_SAMPLE_RATE);
    if (dev.conf.sample_rate != MPU_SAMPLE_RATE) {
        DEBUG("[Error] The sample rate was not set correctly");
    }
    mpu9250_set_compass_sample_rate(&dev, COMPASS_SAMPLE_RATE);
    if (dev.conf.compass_sample_rate != COMPASS_SAMPLE_RATE) {
        DEBUG("[Error] The compass sample rate was not set correctly");
    }
}
/**
 * Internal helper function for calculating measurement delta for WoM.
 */
static uint8_t calc_measurement_delta(mpu9250_results_t *n, mpu9250_results_t *o)
{
    int sum = (n->x_axis - o->x_axis) + (n->y_axis - o->y_axis) + (n->z_axis - o->z_axis);
    if (abs(sum) > DELTA_EPSILON) {
        return 0;
    }
    return 1;
}

/**
 * Internal function for sending MPU data between threads.
 *
 * This blocks on the sender side, but BLE is free
 * to carry out operations during it.
 * XXX: Transform into double buffer static memory.
 */
static void mpu_long_send(  kernel_pid_t target,
                            mpu9250_results_t *accel_data,
                            mpu9250_results_t *gyro_data,
                            mpu9250_results_t *comp_data,
                            uint16_t short_uuid)
{
    msg_t m;
    m.type = MESSAGE_LONG_SEND_1;
    m.content.value = (uint32_t)accel_data->x_axis | ((uint32_t)accel_data->y_axis << 16);
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_2;
    m.content.value = (uint32_t)accel_data->z_axis | ((uint32_t)gyro_data->x_axis << 16);
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_3;
    m.content.value = (uint32_t)gyro_data->y_axis | ((uint32_t)gyro_data->z_axis << 16);
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_4;
    m.content.value = (uint32_t)comp_data->x_axis | ((uint32_t)comp_data->y_axis << 16);
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_5;
    m.content.value = (uint32_t)comp_data->z_axis | ((uint32_t)short_uuid << 16);
    msg_send(&m, target);
}

static msg_t rcv_queue[MPU_RCV_QUEUE_SIZE];

/**
 * MPU thread function. Created by mpu_neppi_init().
 */
NORETURN static void *mpu_thread(void *arg)
{
    (void)arg;
    DEBUG("MPU thread started, pid: %" PRIkernel_pid "\n", thread_getpid());
    msg_t m;
    msg_t m_to_main;
    msg_init_queue(rcv_queue, MPU_RCV_QUEUE_SIZE);
    // Wait until mpu_neppi_start() has been called in main.
    for (;;) {
        msg_receive(&m);
        if (m.type == MESSAGE_MPU_THREAD_START) {
            break;
        }
    }

    // MPU thread execution start

    // Initialize a gpio interrupt for MPU.
    gpio_init_int(MPU_INT_PIN, GPIO_IN_PD, GPIO_RISING, mpu_interrupt, 0);
    // Storage for MPU measurements, needed for calculating difference
    // between measurements.
    static mpu9250_results_t acc;
    static mpu9250_results_t acc_old;
    static mpu9250_results_t gyro;
    static mpu9250_results_t gyro_old;
    static mpu9250_results_t comp;
    static mpu9250_results_t comp_old;
    // Storage for interrupt result. Check mpu9250.h in drivers.
    static mpu9250_int_results_t int_result;
    /**
     * Select Wake on Motion wakeup frequency. Higher frequency costs more
     * current. The options are:
     * MPU9250_WOM_0_24HZ
     * MPU9250_WOM_0_49HZ
     * MPU9250_WOM_0_98HZ
     * MPU9250_WOM_1_95HZ
     * MPU9250_WOM_3_91HZ
     * MPU9250_WOM_7_81HZ
     * MPU9250_WOM_15_63HZ
     * MPU9250_WOM_31_25HZ
     * MPU9250_WOM_62_50HZ
     * MPU9250_WOM_125HZ
     * MPU9250_WOM_250HZ
     * MPU9250_WOM_500HZ
     */
    mpu9250_wom_lp_t wom_freq = MPU9250_WOM_7_81HZ;
    // Enable Wake on Motion. MPU will now send an interrupt if moved
    mpu9250_enable_wom(&dev, WOM_THRESHOLD, wom_freq);
    // MPU API main message loop.
    for (;;) {
        msg_receive(&m);
        switch (m.type) {
            // Message came from the MPU interrupt callback
            case MESSAGE_MPU_INTERRUPT:
                // React differently according to MPU state.
                if (mpu_active) {
                    // Read interrupt status
                    mpu9250_read_int_status(&dev, &int_result);
                    if (int_result.raw) {
                        // We are active and have data ready, read it
                        mpu9250_read_accel(&dev, &acc);
                        mpu9250_read_gyro(&dev, &gyro);
                        mpu9250_read_compass(&dev, &comp);
                        // Compare data to old.
                        if (calc_measurement_delta(&acc, &acc_old)
                                && calc_measurement_delta(&gyro, &gyro_old)
                                && calc_measurement_delta(&comp, &comp_old)) {
                            // Data is roughly the same as old data. Increase counter
                            shutdown_count += 1;
                            if (shutdown_count >= SHUTDOWN_COUNT_MAX) {
                                // If counter fills, set new state and actiave WoM
                                DEBUG("WoM active\n");
                                shutdown_count = 0;
                                mpu_active = 0;
                                mpu9250_enable_wom(&dev, WOM_THRESHOLD, wom_freq);
                                // Notify main about this.
                                m_to_main.type = MESSAGE_MPU_SLEEP;
                                msg_send(&m_to_main, main_pid);
                                break;
                            }
                        }
                        else {
                            // Data is different from old, reset counter
                            shutdown_count = 0;
                        }
                        // Send the data to Client.
                        mpu_long_send(target_pid, &acc, &gyro, &comp, uuid);
                        acc_old = acc;
                        gyro_old = gyro;
                        comp_old = comp;
                    }
                }
                else {
                    // WoM is active. We are interested only in WoM interrupts.
                    // Read interrupt status.
                    mpu9250_read_int_status(&dev, &int_result);
                    if (int_result.wom) {
                        // Movement detected. Set state to active and notify main.
                        DEBUG("Motion detected\n");
                        m_to_main.type = MESSAGE_MPU_ACTIVE;
                        msg_send(&m_to_main, main_pid);
                        mpu_active = 1;
                        // Reset and reconfigure MPU
                        mpu9250_reset_and_init(&dev);
                        set_sample_rates();
                        mpu9250_set_interrupt(&dev, 1);
                    }
                }
                break;
            default:
                break;
        }
    }
}

static char mpu_thread_stack[(THREAD_STACKSIZE_DEFAULT*2)];
/**
 * API function to initialize mpu drivers and the thread running them.
 */
void mpu_neppi_init(kernel_pid_t main, kernel_pid_t target, uint16_t short_uuid)
{
    main_pid = main;
    target_pid = target;
    uuid = short_uuid;
    // Initialize the MPU for the first time.
    int result = mpu9250_init(&dev, &mpu9250_params[0]);
    if (result == -1) {
        DEBUG("[Error] The given i2c is not enabled");
    }
    else if (result == -2) {
        DEBUG("[Error] The compass did not answer correctly on the given address");
    }
    set_sample_rates();
    mpu_thread_pid = thread_create(mpu_thread_stack, sizeof(mpu_thread_stack),
                   THREAD_PRIORITY_MAIN - 2, 0/*THREAD_CREATE_STACKTEST*/,
                   mpu_thread, NULL, "MPU");
}
/**
 * API function to start MPU execution
 */
void mpu_neppi_start(void)
{
    msg_t start_message;
    start_message.type = MESSAGE_MPU_THREAD_START;
    msg_send(&start_message, mpu_thread_pid);
}

