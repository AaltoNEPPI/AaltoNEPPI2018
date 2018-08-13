/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is public domain here.
 */
#ifndef NEPPIBLEH
#define NEPPIBLEH

#include "debug.h"
#include "msg.h"

#define CHANGE_COLOR 777

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Struct to hold characteristic creation data.
 */
typedef struct {
    int data;
} char_descr_t;

/**
 * Initialization function for the BLE API.
 */
void ble_neppi_init(kernel_pid_t);
/**
 * Function to add characteristics. Call this only after ble_neppi_init()
 * and before ble_neppi_start()
 */
uint8_t ble_neppi_add_char(uint16_t UUID, char_descr_t descriptions, uint16_t initial_value);
/**
 * Starts BLE execution.
 */
void ble_neppi_start(void);
/**
 * Function to update a characteristic value.
 */
void ble_neppi_update_char(uint16_t UUID, uint16_t new_value);

#ifdef __cplusplus
}
#endif

#endif //Include guard
