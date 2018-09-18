/*
 * Copyright (C) 2018 Aalto University
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_cap_touch Capacitive touch
 * @ingroup     drivers_actuators
 * @brief       Driver for simple capacitive touch button
 * @{
 *
 * @file
 * @brief       Interface for a simple capacitive touch button
 *
 * @author      Pekka Nikander <pekka.nikander@iki.fi>
 */

#ifndef CAP_TOUCH_H
#define CAP_TOUCH_H

#include "periph/gpio.h"
#include "periph/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Configuration parameters for capacitive touch button
 */
typedef struct {
    gpio_t    sense_pin;       /**< Sensing pin. */
    gpio_t    ground_pin;      /**< Through resistor grounded pin. */
    gpio_t    hack_pin;        /**< Additional grounded pin for hacking. */
    adc_res_t res;             /**< ADC resolution. */
    int       sample_rounds;   /**< Number of averaging rounds */
    int       calib_rounds;    /**< Number of rounds for calibration */
    uint32_t  charging_time;   /**< Pin charging time in us */
    uint32_t  waiting_time;    /**< Time to wait before measuring, in us */
    uint32_t  hysteresis;      /**< State change hysteresis */
    uint16_t  low_bound;       /**< Lower bound for non-touching state */
    uint16_t  high_bound;      /**< Higher bound for non-touching state */
} cap_touch_params_t;

/**
 * @brief   Device descriptor definition for capacitive touch button
 */
typedef cap_touch_params_t cap_touch_t;

/**
 * @brief   Current state or state change in the cap touch button
 */
typedef enum cap_touch_button_state {
    OFF       = 0,
    ON        = 1,
} cap_touch_button_state_t;

/**
 * @brief   Signature for state change interrupt
 *
 * @param[in] state  state or state change
 * @param[in] data   device dependent raw data
 * @param[in] arg    context to the callback (optional)
 */
typedef void(*cap_touch_cb_t)(
    cap_touch_button_state_t state,
    uint16_t data,
    void *arg);

/**
 * @brief   Initialize a capacitive touch button
 *
 * @param[out] dev      device descriptor
 * @param[in]  params   device configuration
 *
 * @pre     @p dev != NULL
 * @pre     @p params != NULL
 *
 * @return  Zero on success, negative number on error
 */
int cap_touch_init(cap_touch_t *dev, const cap_touch_params_t *params);

/**
 * @brief   Calibrate a capacitive touch button
 *
 * @param[out] dev      device descriptor
 *
 * @pre     @p dev != NULL
 */
void cap_touch_calibrate(cap_touch_t *dev);

/**
 * @brief   Start capacitive touch button callbacks.
 *
 * @param[in]  dev    device descriptor
 * @param[in]  cb     state change callback, executed in interrupt context
 * @param[in]  arg    optional context passed to the callback functions
 * @param[in]  flags  options
 *
 * @pre     @p dev != NULL
 * @pre     @p cb != NULL
 */
void cap_touch_start(cap_touch_t *dev, cap_touch_cb_t cb, void *arg, int flags);

/**
 * @brief  Flags for cap_touch_start()
 */
#define CAP_TOUCH_START_FLAG_REPORT_ALL 0x01 //< Report also non-transition events

#ifdef __cplusplus
}
#endif

#endif /* CAP_TOUCH_H */
/** @} */
