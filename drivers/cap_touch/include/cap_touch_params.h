/*
 * Copyright (C) 2018 Aalto University
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_cap_touch
 * @{
 *
 * @file
 * @brief       Simple capacitive touch button driver
 *
 * @author      Pekka Nikander <pekka.nikander@iki.fi>
 */

#ifndef CAP_TOUCH_PARAMS_H
#define CAP_TOUCH_PARAMS_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Set default configuration parameters for the CAP_TOUCH driver
 * @{
 */
#ifndef CAP_TOUCH_PARAM_SENSE_PIN
#define CAP_TOUCH_PARAM_SENSE_PIN       (GPIO_PIN(0, 2))
#endif

#ifndef CAP_TOUCH_PARAMS
#define CAP_TOUCH_PARAMS {                      \
    .sense_pin  = CAP_TOUCH_PARAM_SENSE_PIN,    \
    .res = ADC_RES_14BIT,                       \
    .sample_rounds = 10,                        \
    .calib_rounds = 100,			\
    .grounding_time = 1000,                     \
    .hysteresis = 3,                            \
}
#endif
/**@}*/

/**
 * @brief   CAP_TOUCH configuration
 */
static const  cap_touch_params_t cap_touch_params[] = {
    CAP_TOUCH_PARAMS,
};

#ifdef __cplusplus
}
#endif

#endif /* CAP_TOUCH_PARAMS_H */
/** @} */
