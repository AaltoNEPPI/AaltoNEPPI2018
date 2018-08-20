#ifndef MPUDRIVERH
#define MPUDRIVERH

#define MESSAGE_MPU_THREAD_START    744
#define MESSAGE_LONG_SEND_X          743
#define MESSAGE_LONG_SEND_Y          742
#define MESSAGE_LONG_SEND_Z          741
#define MESSAGE_LONG_SEND_UUID       740
#include "mpu9250.h"
#include "mpu9250_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the MPU9250 drivers and a thread to run them.
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
