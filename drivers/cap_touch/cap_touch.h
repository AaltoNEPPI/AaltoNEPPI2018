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
    adc_res_t res;             /**< ADC resolution. */
    int       sample_rounds;   /**< Number of averaging rounds */
    int       calib_rounds;    /**< Number of rounds for calibration */
    uint32_t  grounding_time;  /**< Pin grounding time in us */
    uint32_t  hysteresis;      /**< State change hysteresis */
} cap_touch_params_t;

/**
 * @brief   Device descriptor definition for capacitive touch button
 */
typedef cap_touch_params_t cap_touch_t;

/**
 * @brief   Signature for state change interrupt
 *
 * @param[in] arg           context to the callback (optional)
 * @param[in] data          new state (zero for no touch, non-zero for finger near or touching)
 */
typedef void(*cap_touch_cb_t)(void *arg, uint8_t data);


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
 * @brief   Start capacitive touch button callbacks.
 *
 * @param[out] dev      device descriptor
 * @param[in]  cb       state change callback, executed in interrupt context
 * @param[in]  arg      optional context passed to the callback functions
 *
 * @pre     @p dev != NULL
 * @pre     @p cb != NULL
 */
void cap_touch_start(cap_touch_t *dev, cap_touch_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* CAP_TOUCH_H */
/** @} */
