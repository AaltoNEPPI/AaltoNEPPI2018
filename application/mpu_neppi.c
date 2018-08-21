#include "mpu_neppi.h"
#define ENABLE_DEBUG (1)
#include "debug.h"
#include "periph/gpio.h"
#include "xtimer.h"

#define MPU_RCV_QUEUE_SIZE  (8)
#define MPU_INT_PIN GPIO_PIN(0,27)
#define MESSAGE_MPU_INTERRUPT       885
// MPU accelerometer and gyro sample rate. Must be between 4 and 1000
#define MPU_SAMPLE_RATE     5
// Compass sample rate. Must be between 1 and 100
#define COMPASS_SAMPLE_RATE     5
static mpu9250_t dev;
static mpu9250_results_t measurement;
static kernel_pid_t target_pid;
static kernel_pid_t main_pid;
static kernel_pid_t mpu_thread_pid;
static uint16_t gyro_uuid;
static uint8_t mpu_active;
/**
 * MPU interrupt callback function
 */
static void mpu_interrupt(void *arg) {
    msg_t m;
    m.type = MESSAGE_MPU_INTERRUPT;
    msg_try_send(&m, mpu_thread_pid);
}

/**
 * Internal helper function for initializing the mpu.
 */
static void config_mpu(void)
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
 * Function for sending MPU data between threads.
 */
static void mpu_long_send(kernel_pid_t target, mpu9250_results_t *mpu_data, uint16_t short_uuid)
{
    //TODO: This could be packed into 2 messages.
    msg_t m;
    m.type = MESSAGE_LONG_SEND_X;
    m.content.value = (uint16_t)mpu_data->x_axis;
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_Y;
    m.content.value = (uint16_t)mpu_data->y_axis;
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_Z;
    m.content.value = (uint16_t)mpu_data->z_axis;
    msg_send(&m, target);
    m.type = MESSAGE_LONG_SEND_UUID;
    m.content.value = short_uuid;
    msg_send(&m, target);
}

static msg_t rcv_queue[MPU_RCV_QUEUE_SIZE];

NORETURN static void *mpu_thread(void *arg)
{
    (void)arg;
    DEBUG("MPU thread started, pid: %" PRIkernel_pid "\n", thread_getpid());
    msg_t m;
    msg_init_queue(rcv_queue, MPU_RCV_QUEUE_SIZE);
    for (;;) {
        msg_receive(&m);
        if (m.type == MESSAGE_MPU_THREAD_START) {
            break;
        }
    }
    // Enable interrupts just before the final message loop.
    gpio_init_int(MPU_INT_PIN, GPIO_IN_PD, GPIO_RISING, mpu_interrupt, 0);
    //mpu9250_set_interrupt(&dev, 1);
    mpu9250_wom_lp_t wom_freq = MPU9250_WOM_7_81HZ;
    mpu9250_enable_wom(&dev, 100, wom_freq);
    for (;;) {
        msg_receive(&m);
        switch (m.type) {
            case MESSAGE_MPU_INTERRUPT:
                ;
                mpu9250_int_results_t int_result;
                if (mpu_active) {
                    mpu9250_read_int_status(&dev, &int_result);
                    if (int_result.raw) {
                        DEBUG("raw: %d, wom: %d\n",int_result.raw, int_result.wom);
                        mpu9250_read_gyro(&dev, &measurement);
                        mpu9250_results_t temp = measurement;
                        mpu_long_send(target_pid, &temp, gyro_uuid);
                    }
                }
                
                else {
                    if (int_result.wom) {
                        mpu_active = 1;
                        mpu9250_reset_and_init(&dev);
                        config_mpu();
                        xtimer_sleep(1);
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
void mpu_neppi_init(kernel_pid_t main, kernel_pid_t target, uint16_t uuid)
{
    main_pid = main;
    target_pid = target;
    gyro_uuid = uuid;
    int result = mpu9250_init(&dev, &mpu9250_params[0]);
    if (result == -1) {
        DEBUG("[Error] The given i2c is not enabled");
    }
    else if (result == -2) {
        DEBUG("[Error] The compass did not answer correctly on the given address");
    }
    //config_mpu();
    mpu_thread_pid = thread_create(mpu_thread_stack, sizeof(mpu_thread_stack),
                   THREAD_PRIORITY_MAIN - 2, 0/*THREAD_CREATE_STACKTEST*/,
                   mpu_thread, NULL, "MPU");
}

void mpu_neppi_start(void)
{
    msg_t start_message;
    start_message.type = MESSAGE_MPU_THREAD_START;
    msg_send(&start_message, mpu_thread_pid);
}

