/*
 * Copyright (C) 2018 Aalto University
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    boards_neppi2018 Aalto NEPPI 2018
 * @ingroup     boards
 * @brief       Support for the Aalto NEPPI 2018
 * @{
 *
 * @file
 * @brief       Board specific configuration for the nRF52 based Aalto NEPPI
 *
 * @author      Pekka Nikander <pekka.nikander@iki.fi>
 */

#ifndef BOARD_H
#define BOARD_H

#include "board_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    LED pin configuration
 * @{
 */
#define LED0_PIN            GPIO_PIN(0, 16)

#define LED_PORT            (NRF_P0)
#define LED0_MASK           (1 << 16)
#define LED_MASK            (LED0_MASK)

#define LED0_ON             (LED_PORT->OUTSET = LED0_MASK)
#define LED0_OFF            (LED_PORT->OUTCLR = LED0_MASK)
#define LED0_TOGGLE         (LED_PORT->OUT   ^= LED0_MASK)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
