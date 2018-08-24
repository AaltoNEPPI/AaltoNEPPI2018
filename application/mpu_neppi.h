
#ifndef MPUDRIVERH
#define MPUDRIVERH

#define MESSAGE_MPU_SLEEP           746
#define MESSAGE_MPU_ACTIVE          745
#define MESSAGE_MPU_THREAD_START    744
#define MESSAGE_LONG_SEND_1         743
#define MESSAGE_LONG_SEND_2         742
#define MESSAGE_LONG_SEND_3         741
#define MESSAGE_LONG_SEND_4         740
#define MESSAGE_LONG_SEND_5         739

#include "mpu9250.h"
#include "mpu9250_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the MPU9250 drivers and a thread to run them.
 *
 * Target is the thread that you want to receive the MPU data. Short_uuid
 * is the uuid of the characteristic you want to contain the MPU data.
 * Make that the characteristic can hold 18 bytes of data.
 */
void mpu_neppi_init(kernel_pid_t main, kernel_pid_t target, uint16_t short_uuid);

/**
 * Starts the MPU9250 thread operation.
 */
void mpu_neppi_start(void);

#ifdef __cplusplus
}
#endif

#endif //Include guard
