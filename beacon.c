/*
 * $ Copyright YEAR Cypress Semiconductor $
 */

/** @file
*
* Beacon sample
*
* This app demonstrates use of Google Eddystone and Apple iBeacons via the
* beacon library. It also demonstrates uses of multi-advertisement feature.
*
* During initialization the app configures advertisement packets for Eddystone and iBeacon
* and starts advertisements via multi-advertisement APIs.
* It also sets up a 1 sec timer to update Eddystone TLM advertisement data
*
* Features demonstrated
*  - configuring Apple iBeacon & Google Eddystone advertisements
*  - Apple iBeacon & Google Eddystone adv message will be advertised simultaneously.
*  - OTA Firmware Upgrade
*
* To demonstrate the app, work through the following steps.
*
* 1. Plug the AIROC eval board into your computer
* 2. Build and download the application to the AIROC board
* 3. Monitor advertisement packets -
*        - on Android, download  app such as 'Beacon Scanner' by Nicholas Briduox
*        - on iOS, download app such as 'Locate Beacon'. Add UUID for iBeacon (see below UUID_IBEACON)
*        - Use over the air sniffer
*
*/
#include "wiced_bt_stack.h"
#include "wiced_memory.h"
#include "wiced_timer.h"
#include "beacon_gatt.h"
#include "wiced_bt_beacon.h"
#include "stdio.h"
#include "stdlib.h"
#include "inttypes.h"

/******************************************************************************
 *                                Defines
 ******************************************************************************/
#define BEACON_CNT 5

/* Stack size */
#define APP_HEAP_SIZE      (1024 * 8)

/* User defined UUID for iBeacon */
#define UUID_IBEACON     0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f

/* Adv parameter defines */
#define PARAM_EVENT_PROPERTY    (WICED_BT_BLE_EXT_ADV_EVENT_CONNECTABLE_ADV|WICED_BT_BLE_EXT_ADV_EVENT_SCANNABLE_ADV|WICED_BT_BLE_EXT_ADV_EVENT_LEGACY_ADV)    // wiced_bt_ble_ext_adv_event_property_t
#define PARAM_FILTER_POLICY     (BTM_BLE_ADV_POLICY_ACCEPT_CONN_AND_SCAN)       // wiced_bt_ble_advert_filter_policy_t
#define PARAM_TX_POWER_MAX      0x7f                                            // tBTM_BLE_ADV_TX_POWER
#define PARAM_SCAN_REQ_NOTIF    WICED_BT_BLE_EXT_ADV_SCAN_REQ_NOTIFY_ENABLE     // wiced_bt_ble_ext_adv_scan_req_notification_setting_t

#define beacon_idx(i) (i-1)
#define beacon_adv_id(i) (i+1)
#define valid_instance(i) (i <= supported_adv)

#define beacon_set_instance_params(instance, interval)   wiced_bt_ble_set_ext_adv_parameters(\
    instance,                           /* wiced_bt_ble_ext_adv_handle_t adv_handle */ \
    PARAM_EVENT_PROPERTY,               /* wiced_bt_ble_ext_adv_event_property_t event_properties */ \
    interval,                           /* uint32_t primary_adv_int_min */ \
    interval,                           /* uint32_t primary_adv_int_max */ \
    BTM_BLE_DEFAULT_ADVERT_CHNL_MAP,    /* wiced_bt_ble_advert_chnl_map_t primary_adv_channel_map */ \
    BLE_ADDR_RANDOM,                    /* wiced_bt_ble_address_type_t own_addr_type */ \
    BLE_ADDR_RANDOM,                    /* wiced_bt_ble_address_type_t peer_addr_type */ \
    peer_addr,                          /* wiced_bt_device_address_t peer_addr */ \
    PARAM_FILTER_POLICY,                /* wiced_bt_ble_advert_filter_policy_t adv_filter_policy */ \
    PARAM_TX_POWER_MAX,                 /* int8_t adv_tx_power */ \
    WICED_BT_BLE_EXT_ADV_PHY_1M,        /* wiced_bt_ble_ext_adv_phy_t primary_adv_phy */ \
    0,                                  /* uint8_t secondary_adv_max_skip */ \
    0,                                  /* wiced_bt_ble_ext_adv_phy_t secondary_adv_phy */ \
    0,                                  /* wiced_bt_ble_ext_adv_sid_t adv_sid */ \
    PARAM_SCAN_REQ_NOTIF);              /* wiced_bt_ble_ext_adv_scan_req_notification_setting_t scan_request_not */

/******************************************************************************
 *                                Structures
 ******************************************************************************/
typedef uint8_t beacon_adv_data_t[WICED_BT_BEACON_ADV_DATA_MAX];
typedef uint8_t eddystone_namespace_t[EDDYSTONE_UID_NAMESPACE_LEN];
typedef uint8_t eddystone_instance_t[EDDYSTONE_UID_INSTANCE_ID_LEN];
typedef uint8_t eddystone_eid_data_t[EDDYSTONE_EID_LEN];
typedef uint8_t eddystone_encoded_url_t[EDDYSTONE_URL_VALUE_MAX_LEN];
typedef uint8_t eddystone_etlm_t[EDDYSTONE_ETLM_LEN];
typedef uint8_t (set_data_func_t)(beacon_adv_data_t adv_data);
typedef struct
{
    set_data_func_t * set_data;
    uint32_t interval;
    uint8_t  id;
} beacon_adv_t;


/******************************************************************************
 *                              Variables Definitions
 ******************************************************************************/
static uint8_t                                  supported_adv;
static wiced_bt_device_address_t                peer_addr = {0,0,0,0,0,0};
static uint16_t                                 beacon_conn_id = 0;
static wiced_bt_db_hash_t                       beacon_db_hash;
static wiced_bt_ble_ext_adv_duration_config_t   duration_cfg[BEACON_CNT] = {{1},{2},{3},{4},{5}};
static wiced_timer_t                            beacon_timer;
static uint8_t                                  adv_idx = 0;

extern const wiced_bt_cfg_settings_t app_cfg_settings;
/******************************************************************************
 *     Private Function Definitions
 ******************************************************************************/

/*
 * This function prints the bd_address
 */
void print_bd_address(wiced_bt_device_address_t bdadr)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X\n",bdadr[0],bdadr[1],bdadr[2],bdadr[3],bdadr[4],bdadr[5]);
}

/*
 * This function prepares Google Eddystone UID advertising data
 */
static uint8_t beacon_set_eddystone_uid_advertisement_data(beacon_adv_data_t adv_data)
{
    /* Set sample values for Eddystone UID*/
    uint8_t len;
    uint8_t ranging_data = 0xf0;
    eddystone_namespace_t namespace = { 1,2,3,4,5,6,7,8,9,0 };
    eddystone_instance_t instance = { 0,1,2,3,4,5 };

    /* Call Eddystone UID api to prepare adv data*/
    wiced_bt_eddystone_set_data_for_uid(ranging_data, namespace, instance, adv_data, &len);
    return len;
}

/*
* This function prepares Google Eddystone URL advertising data
*/
static uint8_t beacon_set_eddystone_url_advertisement_data(beacon_adv_data_t adv_data)
{
    /* Set sample values for Eddystone URL*/
    uint8_t len;
    uint8_t beacon_tx_power = 0x01;
    eddystone_encoded_url_t encoded_url = "infineon.com";

    /* Call Eddystone URL api to prepare adv data*/
    wiced_bt_eddystone_set_data_for_url(beacon_tx_power, EDDYSTONE_URL_SCHEME_0, encoded_url, adv_data, &len);
    return len;
}

/*
* This function prepares Google Eddystone EID advertising data
*/
static uint8_t beacon_set_eddystone_eid_advertisement_data(beacon_adv_data_t adv_data)
{
    /* Set sample values for Eddystone EID*/
    uint8_t len;
    uint8_t ranging_data = 0xf0;
    eddystone_eid_data_t eid = { 0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8 };

    /* Call Eddystone EID api to prepare adv data*/
    wiced_bt_eddystone_set_data_for_eid(ranging_data, eid, adv_data, &len);
    return len;
}

/*
* This function prepares Google Eddystone TLM advertising data
*/
static uint8_t beacon_set_eddystone_tlm_advertisement_data(beacon_adv_data_t adv_data)
{
    /* Set sample values for Eddystone TLM */
    uint8_t len;
    uint16_t vbatt = 10;
    uint16_t temp =  15;
    static uint32_t adv_cnt = 0;
    static uint32_t sec_cnt = 0;

    /* For each invocation of API, update Advertising PDU count and Time since power-on or reboot*/
    adv_cnt++;
    sec_cnt++;

    /* Call Eddystone TLM api to prepare adv data*/
    wiced_bt_eddystone_set_data_for_tlm_unencrypted(vbatt, temp, adv_cnt, sec_cnt, adv_data, &len);
    return len;
}

/*
* This function prepares Apple iBeacon advertising data
*/
static uint8_t beacon_set_ibeacon_advertisement_data(beacon_adv_data_t adv_data)
{
    /* Set sample values for iBeacon */
    uint8_t len;
    uint8_t ibeacon_uuid[LEN_UUID_128] = { UUID_IBEACON };
    uint16_t ibeacon_major_number = 0x01;
    uint16_t ibeacon_minor_number = 0x02;
    uint8_t tx_power_lcl = 0xb3;

    printf("beacon_set_ibeacon_advertisement_data\n");

    /* Call iBeacon api to prepare adv data*/
    wiced_bt_ibeacon_set_adv_data(ibeacon_uuid, ibeacon_major_number, ibeacon_minor_number, tx_power_lcl, adv_data, &len);
    return len;
}

/*
 * beacon_adv_t
 */
static beacon_adv_t adv[BEACON_CNT] =
{
    {beacon_set_ibeacon_advertisement_data,        160},
    {beacon_set_eddystone_uid_advertisement_data,  320},
    {beacon_set_eddystone_url_advertisement_data,   80},
    {beacon_set_eddystone_eid_advertisement_data,  480},
    {beacon_set_eddystone_tlm_advertisement_data, 1280},
};

static void beacon_start(uint8_t instance, uint8_t idx)
{
    wiced_bt_device_address_t  random_bda = {0x40, 0x01, 0x02, 0x03, 0x04, 0x05};
    beacon_adv_data_t buff;
    uint8_t len;

    printf("beacon_start instance %d for index %d\n", instance, idx);
    adv[idx].id = instance;
    beacon_set_instance_params(instance, adv[idx].interval);
    random_bda[1] = idx; // make address unique
    wiced_bt_ble_set_ext_adv_random_address (instance, random_bda);
    len = adv[idx].set_data(buff);

    /* Sets adv data for this instance & start to adv */
    wiced_bt_ble_set_ext_adv_data(instance, len, buff);
    wiced_bt_ble_start_ext_adv(MULTI_ADVERT_START, 1, &duration_cfg[beacon_idx(instance)]);
}

/*
 * This function stops the adv when the instance is in use. It returns the freed instance.
 * If it was not in adv, it searches for next avaiable free instance and return the free instance.
 * When free instance is no available, it returns value larger than supported_adv
 */
static uint8_t beacon_stop(uint8_t idx)
{
    uint8_t instance = adv[idx].id;

    // check if it is in use (advertizing)
    if (instance)
    {
        printf("beacon_stop instance %d\n", instance);
        adv[idx].id = 0;    // mark as adv stopped
        wiced_bt_ble_start_ext_adv(MULTI_ADVERT_STOP, 1, &duration_cfg[beacon_idx(instance)]);
    }
    else
    {
        // find the next available free instance.
        // The instance is free only if there is no adv is using this instance.
        for (instance=1; instance<=supported_adv; instance++)
        {
            int i;

            // instance is avaiable if it is not in use
            for (i=0; i<BEACON_CNT; i++)
            {
                if (adv[i].id == instance) // in use?
                {
                    break;
                }
            }

            // if the instance is not in use for all advertisement, we found free instance
            if (i>=BEACON_CNT)
            {
                break;
            }
        }
    }
    return instance;
}

/*
 * This function beacon_data_update
 */
static void beacon_switch_adv(WICED_TIMER_PARAM_TYPE arg)
{
    uint8_t instance;
    uint8_t stop_idx = adv_idx;
    uint8_t start_idx = stop_idx + supported_adv;

    if (start_idx >= BEACON_CNT)
    {
        start_idx -= BEACON_CNT;
    }
    if (++adv_idx >= BEACON_CNT)
    {
        adv_idx = 0;
    }

    instance = beacon_stop(stop_idx);
    if (valid_instance(instance))
    {
        beacon_start(instance, start_idx);
    }
    else
    {
        printf("No free instance\n");
    }
}

/*
 * This function set a timer which will change Eddystone TLM advertising data
 * on every interval(1 sec) and rotates the adv within available sets.
 */
static void beacon_set_timer(void)
{
    /* init beacon timer */
    wiced_init_timer ( &beacon_timer, beacon_switch_adv, 0, WICED_SECONDS_PERIODIC_TIMER );

    // start timer
    wiced_start_timer( &beacon_timer, 1 );
}

/*
 * Initialize for Beacon adv
 */
static void beacon_adv_init()
{
    printf("beacon_adv_init\n");

    /* Set the advertising params and make the device discoverable */
    beacon_set_app_advertisement_data();
    wiced_bt_start_advertisements( BTM_BLE_ADVERT_UNDIRECTED_HIGH, 0, NULL );

    supported_adv = wiced_bt_ble_read_num_ext_adv_sets();
    if (supported_adv > BEACON_CNT)
    {
        supported_adv = BEACON_CNT;
    }

    printf("Supported adv set: %d\n", supported_adv);

    // start adv.
    for (int idx=0; idx<supported_adv; idx++)
    {
        beacon_start( beacon_adv_id(idx), idx );
    }

    /* start timer to change beacon ADV data */
    beacon_set_timer();
}

/*
 * This function is executed in the BTM_ENABLED_EVT management callback.
 */
static void beacon_init(void)
{
    wiced_bt_gatt_status_t gatt_status;

    /* Register with stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(beacon_gatts_callback);

    printf("wiced_bt_gatt_register: %d\n", gatt_status);

    /*  Tell stack to use our GATT database */
    gatt_status =  wiced_bt_gatt_db_init( gatt_database, gatt_database_len, beacon_db_hash );

    printf("wiced_bt_gatt_db_init %d\n", gatt_status);

    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(WICED_TRUE, 0);
    beacon_adv_init();
}

/*
 * This function is invoked when advertisements stop.  If we are configured to stay connected,
 * disconnection was caused by the peer, start low advertisements, so that peer can connect
 * when it wakes up
 */
static void beacon_advertisement_stopped(void)
{
    wiced_result_t result;

    // while we are not connected
    if (beacon_conn_id == 0)
    {
        result =  wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_LOW, 0, NULL);
        printf("wiced_bt_start_advertisements: %d\n", result);
    }
    else
    {
        printf("ADV stop\n");
    }
}

/*
 * Application management callback.  Stack passes various events to the function that may
 * be of interest to the application.
 */
static wiced_result_t beacon_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t                    result = WICED_BT_SUCCESS;
//    uint8_t                          *p_keys;
    wiced_bt_ble_advert_mode_t       *p_mode;

    printf("beacon_management_callback: %x\n", event);

    switch(event)
    {
    /* Bluetooth  stack enabled */
    case BTM_ENABLED_EVT:
        beacon_init();
        break;

    case BTM_DISABLED_EVT:
        break;

    case BTM_USER_CONFIRMATION_REQUEST_EVT:
        printf("Numeric_value: %"PRIu32" \n", p_event_data->user_confirmation_request.numeric_value);
        wiced_bt_dev_confirm_req_reply( WICED_BT_SUCCESS , p_event_data->user_confirmation_request.bd_addr);
        break;

    case BTM_PASSKEY_NOTIFICATION_EVT:
        printf("\r\n  PassKey Notification from BDA: ");
        print_bd_address(p_event_data->user_passkey_notification.bd_addr);
        printf("PassKey: %"PRIu32" \n", p_event_data->user_passkey_notification.passkey );
        wiced_bt_dev_confirm_req_reply(WICED_BT_SUCCESS, p_event_data->user_passkey_notification.bd_addr );
        break;

    case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        printf("BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:\n" );
        p_event_data->pairing_io_capabilities_ble_request.local_io_cap  = BTM_IO_CAPABILITIES_NONE;
        p_event_data->pairing_io_capabilities_ble_request.oob_data      = BTM_OOB_NONE;
        p_event_data->pairing_io_capabilities_ble_request.auth_req      = BTM_LE_AUTH_REQ_BOND | BTM_LE_AUTH_REQ_MITM;
        p_event_data->pairing_io_capabilities_ble_request.max_key_size  = 0x10;
        p_event_data->pairing_io_capabilities_ble_request.init_keys     = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
        p_event_data->pairing_io_capabilities_ble_request.resp_keys     = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
        break;

    case BTM_SECURITY_REQUEST_EVT:
        wiced_bt_ble_security_grant( p_event_data->security_request.bd_addr, WICED_BT_SUCCESS );
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
        break;

    case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
        break;

    case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
        break;

    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        p_mode = &p_event_data->ble_advert_state_changed;
        printf("Advertisement State Change: %d\n", *p_mode);
        if (*p_mode == BTM_BLE_ADVERT_OFF)
        {
            beacon_advertisement_stopped();
        }
        break;

    default:
        printf("Not handled\n");
        break;
    }

    return result;
}

/******************************************************************************
 *     Public Function Definitions
 ******************************************************************************/

/*
 * Connection up/down event
 */
wiced_bt_gatt_status_t beacon_connection_status_event(wiced_bt_gatt_connection_status_t *p_status)
{
    wiced_result_t result;

    if (p_status->connected)
    {
        beacon_conn_id = p_status->conn_id;
        wiced_bt_start_advertisements(BTM_BLE_ADVERT_OFF, 0, NULL);  // stop adv.
    }
    else
    {
        beacon_conn_id = 0;
        result =  wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, 0, NULL);
        printf("[%s] start adv status %d \n", __FUNCTION__, result);
    }
    return WICED_BT_GATT_SUCCESS;
}

/*
 *  Entry point to the application. Set device configuration and start BT
 *  stack initialization.  The actual application initialization will happen
 *  when stack reports that BT device is ready.
 */
void application_start( void )
{
    printf("application_start A\n");

    wiced_result_t wiced_result;
    // Register call back and configuration with stack
    wiced_result = wiced_bt_stack_init (beacon_management_callback, &app_cfg_settings);

    if( WICED_BT_SUCCESS == wiced_result)
   {
        printf("Bluetooth Stack Initialization Successful \n");
   }
   else
   {
        printf("Bluetooth Stack Initialization failed!! \n");
   }

    printf("application_start B\n");

    printf("Beacon Application Start\n");
}
