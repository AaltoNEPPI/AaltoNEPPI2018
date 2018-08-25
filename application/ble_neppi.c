/*
 * Created by Pekka Nikander and others at Aalto University in 2018.
 *
 * This code has been placed in public domain.
 */

#include <stdio.h>
#include <inttypes.h>

#include <stdbool.h>
#include <stdint.h>
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "board.h"
#include "thread.h"
#include "msg.h"
#include "nrf_ble_gatt.h"
#include "nrf_sdh_ble.h"
#include "app_error.h"
#include "net/gnrc/netif.h"
#include "ble-core.h"
#include "xtimer.h"
#include "board.h"
#include "periph/gpio.h"
#include "mpu_neppi.h"

#define ENABLE_DEBUG (1)
#include "debug.h"
#include "ble_neppi.h"

#define LED_CONNECTED_ON  LED0_ON
#define LED_CONNECTED_OFF LED0_OFF

/**
 * Maximum number of characteristics a service can have.
 *
 * This can come from the sdk ble headers.
 */
//#ifndef BLE_GATT_DB_MAX_CHARS
#define BLE_GATT_DB_MAX_CHARS 6
//#endif

/**
 * RIOT thread priority for the BLE handler.
 *
 * We use the same thread priority as for TCP/IP network interfaces.
 */
#define BLE_THREAD_PRIO (GNRC_NETIF_PRIO)

/**
 * BLE thread message queue size
 */
#define BLE_RCV_QUEUE_SIZE  (8)

/**
 * Advertised device name
 */
#ifndef DEVICE_NAME
#define DEVICE_NAME "Neppi1"
#endif // DEVICE_NAME

/**
 * Application's BLE observer priority.
 *
 * You shouldn't need to modify this value.
 * Used in NRF_SDH_BLE_OBSERVER.
 * By default there are a maximum of four levels, 0-3.
 * By default, the application priority should be lowest (highest number).
 */
#define APP_BLE_OBSERVER_PRIO 3 /* (NRF_SDH_BLE_OBSERVER_PRIO_LEVELS-1) */

/**
 * A tag for the SoftDevice BLE configuration.
 *
 * This is basically any small integer.  By convention,
 * one is used for generic applications.
 */
#define APP_BLE_CONN_CFG_TAG 1

/**************
 * BLE Advertisement parameters
 **************/

/**
 * The advertising interval (in units of 0.625 ms).
 *
 * This value can vary between 100ms to 10.24s).
 */
#define APP_ADV_INTERVAL MSEC_TO_UNITS(50, UNIT_0_625_MS)

/**************
 * BLE GAP parameters
 **************/

/**
 * Minimum acceptable connection interval (0.5 seconds).
 */
#define MIN_CONN_INTERVAL MSEC_TO_UNITS(100, UNIT_1_25_MS)

/**
 * Maximum acceptable connection interval (1 second).
 */
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(200, UNIT_1_25_MS)

/**
 * Slave latency.
 */
#define SLAVE_LATENCY 0

/**
 * Connection supervisory time-out (4 seconds).
 */
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)

/*************
 * Messages from the BLE thread to the main thread
 *************/

#define BLE_THREAD_START    555
#define BLE_DISCONNECT_MSG  444
#define BLE_CONNECT_MSG     333
/*************
 * UUIDs used for the service and characteristics
 *************/

// 128-bit base UUID
#define BLE_UUID_OUR_BASE_UUID \
{{ 0x8D, 0x19, 0x7F, 0x81, \
   0x08, 0x08, 0x12, 0xE0, \
   0x2B, 0x14, 0x95, 0x71, \
   0x05, 0x06, 0x31, 0xB1, \
}}

// Just a random, but recognizable value for the service
#define BLE_UUID_OUR_SERVICE                             0xABDC

/**
 * Variable to keep track of number of characteristics.
 */
static uint8_t char_count = 0;

/**
 * Struct to store service and characteristic related information
 */
typedef struct {
    /**
     * Handle of the current connection (as provided by the BLE stack,
     * is BLE_CONN_HANDLE_INVALID if not in a connection).
     */
    uint16_t conn_handle;
    /**
     * Handle of Our Service (as provided by the BLE stack).
     */
    uint16_t service_handle;
    /**
     * Handle of characteristic (as provided by the BLE stack).
     */
    ble_gatts_char_handles_t char_handles[BLE_GATT_DB_MAX_CHARS];
    /**
     * short UUID of the characteristic. This is not from the BLE stack.
     */
    uint16_t uuids[BLE_GATT_DB_MAX_CHARS];
    /**
     * Characteristic length. Supplied by user.
     */
    uint8_t char_lens[BLE_GATT_DB_MAX_CHARS];
} ble_os_t;

static ble_os_t our_service;

/**************
 * RIOT thread IDs
 **************/

static kernel_pid_t send_pid;
static kernel_pid_t ble_thread_pid;

/**
 * Forward declarations
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);

/**
 * Function called by the NRF_ASSERT macro when an assertion
 * fails somewhere in the NRF SDK.
 *
 * For now, we just print an error message and enter a busy
 * loop, waiting for a developer with a debugger.
 *
 * XXX Note that this is NOT good for a production version.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    core_panic(PANIC_ASSERT_FAIL, "RIOT: NRF Assertion failed.\n");
}

/**
 * A local RIOT-nice error handler replacing NRF's APP_ERROR_CHECK
 */
#define NRF_APP_ERROR_CHECK(err_code)                                           \
do {                                                                            \
  if (NRF_SUCCESS != err_code) {                                                \
    DEBUG("NRF call failed: err=%lu, %s#%d\n", err_code, __FILE__, __LINE__);   \
    return;                                                                     \
  }                                                                             \
} while(0)

static ble_uuid_t adv_uuids[] = {{BLE_UUID_OUR_SERVICE, 0 /* Filled dynamically */}};

//XXX Some of these could come as input from main during initialization.
static const ble_context_t ble_context = {
    .conn_cfg_tag = APP_BLE_CONN_CFG_TAG,
    .name = DEVICE_NAME,
    .adv_uuids = adv_uuids,
    .adv_uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]),
    .app_adv_interval = APP_ADV_INTERVAL,
};

// Register a handler for BLE events.
NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

/**
 * Internal function to initialize our service.
 */
static void services_init(const ble_context_t* p_ble_context)
{
    ble_uuid_t *p_service_uuid = p_ble_context->adv_uuids;
    ble_os_t*   p_our_service = &our_service;

    uint32_t      err_code;
    ble_uuid128_t base_uuid = BLE_UUID_OUR_BASE_UUID;

    /* service_uuid_p->uuid already filled in */
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_service_uuid->type);
    NRF_APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        p_service_uuid,
                                        &p_our_service->service_handle);
    NRF_APP_ERROR_CHECK(err_code);

    p_our_service->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**
 * Function for the GAP initialization.
 *
 * This function sets up all the necessary GAP (Generic Access
 * Profile) parameters of the including the device name, appearance,
 * and the preferred connection parameters.
 */
static void gap_params_init(const ble_context_t* p_ble_context)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (uint8_t*)p_ble_context->name,
                                          strlen(p_ble_context->name));
    NRF_APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    NRF_APP_ERROR_CHECK(err_code);
}


/**
 * Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    NRF_BLE_GATT_DEF(m_gatt);

    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    NRF_APP_ERROR_CHECK(err_code);
}
/**
 * Internal function to add a characteristic.
 *
 * XXX: Initial value is only 8 bit, while value size can be larger.
 * Initial value should be a pointer to a byte array instead, though
 * this shouldn't matter in the scope of our project.
 */
static void add_characteristic(ble_os_t* p_our_service, uint16_t  characteristic, uint8_t value, uint8_t char_len)
{
    // Before we do anything, we need to be sure that we're not full.
    assert(char_count < BLE_GATT_DB_MAX_CHARS);

    ble_uuid128_t base_uuid = BLE_UUID_OUR_BASE_UUID;
    uint32_t      err_code = 0;
    ble_uuid_t    char_uuid;

    char_uuid.uuid = characteristic;
    sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    NRF_APP_ERROR_CHECK(err_code);

    // Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;

    // Configuring Client Characteristic Configuration Descriptor metadata and
    // add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md = &cccd_md;
    char_md.char_props.notify = 1;

    // Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc = BLE_GATTS_VLOC_STACK;

    // Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;

        // Set characteristic length in number of bytes
    attr_char_value.max_len = char_len;
    attr_char_value.init_len = sizeof(value);
    attr_char_value.p_value = (uint8_t*)&value ;

    // Store the original UUID so we can update the characteristic later via the API
    p_our_service->uuids[char_count] = characteristic;

    // Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_our_service->service_handle,
            &char_md,
            &attr_char_value,
            &p_our_service->char_handles[char_count]);
    NRF_APP_ERROR_CHECK(err_code);
    // Store the characteristic length for updating later
    p_our_service->char_lens[char_count] = char_len;
    // We need to keep track of how many characteristics we currently have.
    char_count += 1;
}
/**
 * Function to process miscellaneous softdevice events.
 */
static void on_ble_evt(ble_os_t * p_our_service, ble_evt_t const * p_ble_evt)
{
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id) {
    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(our_service.conn_handle, NULL, 0, 0);
            NRF_APP_ERROR_CHECK(err_code);
            break;
    case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            DEBUG("GATT Client Timeout.\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            NRF_APP_ERROR_CHECK(err_code);
            break;
    case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            DEBUG("GATT Server Timeout.\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                           BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            NRF_APP_ERROR_CHECK(err_code);
            break;
    default:
            break;
    }
}

/**
 * Part of the SoftDevice callback function.
 *
 * This function is how we respond to the softdevice event when a client
 * writes on our server. This function is currently called in an
 * interrupt context and is only thread safe on the assuption that
 * no more characteristics are added after ble thread operation starts.
 * This means after ble_neppi_init() has been called but before ble_neppi_start()
 */
static void on_ble_write(ble_os_t * p_our_service, ble_evt_t const * p_ble_evt)
{
    // Buffer to hold received data. The data can only be at most 32 bit long.
    uint32_t data = 0;

    // Populate ble_gatts_value_t structure for received data and metadata.
    ble_gatts_value_t rx_data = {
        .len = sizeof(data),
        .offset = 0,
    .p_value = (uint8_t *)&data,
    };

    const uint16_t handle = p_ble_evt->evt.gatts_evt.params.write.handle;
    // Check if write event is performed on our characteristic or CCCD.
    for (uint8_t i = 0; i < char_count; i++) {
            if (handle == p_our_service->char_handles[i].value_handle) {
                // Get data
                sd_ble_gatts_value_get(p_our_service->conn_handle, handle, &rx_data);
                DEBUG("Value changed h=%d: %u\n", handle, data);
            msg_t m;
            m.type = p_our_service->uuids[i];
            m.content.value = data;
            msg_try_send(&m, send_pid);
            }
        else if (handle == p_our_service->char_handles[i].cccd_handle) {
                DEBUG("CCCD for h=%d\n", handle);
                // Get data
                sd_ble_gatts_value_get(p_our_service->conn_handle, handle, &rx_data);
            }
    }
}

/**
 * Function to respond to softdevice events that impact our service.
 */
void ble_our_service_on_ble_evt(ble_os_t * p_our_service, ble_evt_t const * p_ble_evt)
{
    // Implement switch case handling BLE events related to our service.
    switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
        ;
        msg_t m1;
        m1.type = BLE_CONNECT_MSG;
        m1.content.value = p_ble_evt->evt.gap_evt.conn_handle;
        msg_try_send(&m1, ble_thread_pid);
            break;
    case BLE_GAP_EVT_DISCONNECTED:
        ;
        msg_t m2;
        m2.type = BLE_DISCONNECT_MSG;
        m2.content.value = p_ble_evt->evt.gap_evt.conn_handle;
        msg_try_send(&m2, ble_thread_pid);
            p_our_service->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;
    case BLE_GATTS_EVT_WRITE:
            on_ble_write(p_our_service, p_ble_evt);
            break;
    default:
        // No implementation needed.
            break;
    }
}
/**
 * Function to update a characteristic.
 *
 * Inputs:
 *  p_our_service:  Pointer to the static service and characteristic struct.
 *  data:           Pointer to the byte array containing our data.
 *  uuid:           Short UUID of the characteristic we want to update.
 */
static void characteristic_update(ble_os_t *p_our_service, uint8_t *data, uint16_t uuid)
{
    // Find our UUID we want to update.
    for (uint8_t i = 0; i < char_count; i++) {
        if (p_our_service->uuids[i] == uuid) {
            // If there is a match, see if device is connected.
            if (p_our_service->conn_handle != BLE_CONN_HANDLE_INVALID) {
                // Device connected, procedure for update + notification.
                    uint16_t               len = p_our_service->char_lens[i];
                    ble_gatts_hvx_params_t hvx_params;
                    memset(&hvx_params, 0, sizeof(hvx_params));
                    hvx_params.handle = p_our_service->char_handles[i].value_handle;
                    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
                    hvx_params.offset = 0;
                    hvx_params.p_len  = &len;
                    hvx_params.p_data = data;

                    sd_ble_gatts_hvx(p_our_service->conn_handle, &hvx_params);
            }
            else {
            // The procedure to change a value without active connection.
                    uint16_t          len = p_our_service->char_lens[i];
                    ble_gatts_value_t tx_data;
                    tx_data.len     = len;
                    tx_data.offset  = 0;
                    tx_data.p_value = data;
                    sd_ble_gatts_value_set(p_our_service->conn_handle,
                                           p_our_service->char_handles[i].value_handle,
                                           &tx_data);
            }
        }
    }
}
/**
 * This function responds to the softdevice callbacks.
 */
static void
ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    on_ble_evt(&our_service, p_ble_evt);
    ble_our_service_on_ble_evt(&our_service, p_ble_evt);
}

static msg_t rcv_queue[BLE_RCV_QUEUE_SIZE];
/**
 * BLE thread function.
 */
NORETURN static void *ble_thread(void *arg)
{
    (void)arg;
    DEBUG("ble_neppi: BLE thread started: pid=%" PRIkernel_pid "\n", thread_getpid());
    // Wait until start message has been sent.
    msg_t m;
    msg_init_queue(rcv_queue, BLE_RCV_QUEUE_SIZE);
    for (;;) {
        msg_receive(&m);
        if (m.type == BLE_THREAD_START) {
            break;
        }
    }
    // Start execution.
    ble_advertising_start(&ble_context);
    static uint8_t mpu_data[18];
    memset(mpu_data, 0, sizeof(mpu_data));
    for (;;) {
            msg_receive(&m);
        // Check if the message is about updating a characteristic.
        for (uint8_t i = 0; i < char_count; i++) {
            if (our_service.uuids[i] == m.type) {
                characteristic_update(&our_service, (uint8_t *)&m.content.value, m.type);
            }
        }
        switch (m.type) {
            /*  Little endian send */
            // XXX Turn into double buffer send.
            case MESSAGE_LONG_SEND_1:
                *((uint32_t *)&mpu_data[0]) = m.content.value;
                break;
            case MESSAGE_LONG_SEND_2:
                *((uint32_t *)&mpu_data[4]) = m.content.value;
                break;
            case MESSAGE_LONG_SEND_3:
                *((uint32_t *)&mpu_data[8]) = m.content.value;
                break;
            case MESSAGE_LONG_SEND_4:
                *((uint32_t *)&mpu_data[12]) = m.content.value;
                break;
            case MESSAGE_LONG_SEND_5:
                *((uint16_t *)&mpu_data[16]) = (uint16_t)m.content.value;
                uint16_t temp_uuid = (uint16_t)(m.content.value >> 16);
                characteristic_update(&our_service, mpu_data, temp_uuid);
                memset(mpu_data, 0, sizeof(mpu_data));
                break;
            
            /*  Big endian send */
            /*
            case MESSAGE_LONG_SEND_1:
                *((uint32_t *)&mpu_data[14]) = htonl(m.content.value);
                break;
            case MESSAGE_LONG_SEND_2:
                *((uint32_t *)&mpu_data[10]) = htonl(m.content.value);
                break;
            case MESSAGE_LONG_SEND_3:
                *((uint32_t *)&mpu_data[6]) = htonl(m.content.value);
                break;
            case MESSAGE_LONG_SEND_4:
                *((uint32_t *)&mpu_data[2]) = htonl(m.content.value);
                break;
            case MESSAGE_LONG_SEND_5:
                *((uint16_t *)&mpu_data[0]) = htons((uint16_t)m.content.value);
                uint16_t temp_uuid = (uint16_t)(m.content.value >> 16);
                characteristic_update(&our_service, mpu_data, temp_uuid);
                memset(mpu_data, 0, sizeof(mpu_data));
                break;
            */
            case BLE_CONNECT_MSG:
                LED_CONNECTED_ON;
                our_service.conn_handle = m.content.value;
                break;
            case BLE_DISCONNECT_MSG:
                LED_CONNECTED_OFF;
                our_service.conn_handle = BLE_CONN_HANDLE_INVALID;
                break;
            default:
                break;
        }
    }
}

static char ble_thread_stack[(THREAD_STACKSIZE_DEFAULT*2)];
/*
 * API function to initialize BLE and the thread that runs it.
 */
kernel_pid_t ble_neppi_init(kernel_pid_t main_pid)
{
    send_pid = main_pid;
    ble_init(&ble_context);
    gap_params_init(&ble_context);
    gatt_init();
    services_init(&ble_context);
    ble_advertising_init(&ble_context);

    ble_thread_pid = thread_create(ble_thread_stack, sizeof(ble_thread_stack),
                                   BLE_THREAD_PRIO, THREAD_CREATE_WOUT_YIELD,
                                   ble_thread, NULL, "BLE");
    DEBUG("ble_neppi: BLE thread created\n");
    return ble_thread_pid;
}
/**
 * API function to start the BLE thread after initialization.
 */
void ble_neppi_start(void)
{
    msg_t start_message;
    start_message.type = BLE_THREAD_START;
    msg_send(&start_message, ble_thread_pid);
}
/**
 * API function to add a new characteristic to our service.
 *
 * Should only be used after initialization, but before thread has been started.
 */
uint8_t ble_neppi_add_char(uint16_t UUID, char_descr_t descriptions, uint8_t initial_value)
{
    if (char_count >= BLE_GATT_DB_MAX_CHARS){
        return 0;
    }
    add_characteristic(&our_service, UUID, initial_value, descriptions.char_len);
    return 1;
}
/*
 * API function to update an added characteristic.
 */
void ble_neppi_update_char(uint16_t UUID, uint32_t value)
{
    msg_t m;
    m.type = UUID;
    m.content.value = value;
    msg_try_send(&m, ble_thread_pid);
}
