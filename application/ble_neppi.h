/**
 * Made by Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
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
 *
 * char_len defines the length of the characteristic in bytes.
 */
typedef struct {
    uint8_t char_len;
} char_descr_t;

/**
 * Initialize the NEPPI BLE subsystem.
 *
 * Initializes the nRF52 SoftDevice and starts a thread to handle it.
 * @param pid          a pid for a thread that shall receive SD notifications
 * @param base_uuid    a 128 bit base UUID
 * @param service_uuid a 16 bit UUID identifying the service within the base
 */
kernel_pid_t ble_neppi_init(
    kernel_pid_t pid, const ble_uuid128_t *base_uuid, uint16_t service_uuid);

/**
 * Function to add characteristics. 
 *
 * Call this only after ble_neppi_init() and before ble_neppi_start()
 */
uint8_t ble_neppi_add_char(uint16_t UUID, char_descr_t descriptions, uint8_t initial_value);
/**
 * Starts BLE execution.
 */
void ble_neppi_start(void);
/**
 * Function to update a characteristic value.
 */
void ble_neppi_update_char(uint16_t UUID, uint32_t new_value);

#ifdef __cplusplus
}
#endif

#endif //Include guard
