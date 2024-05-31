/*
 * $ Copyright YEAR Cypress Semiconductor $
 */

/** @file
*
* Beacon gatt functions
*
*/
#include "wiced_bt_cfg.h"
#include "wiced_bt_gatt.h"
#include "cycfg_gap.h"
#include "cycfg_gatt_db.h"


wiced_bt_gatt_status_t beacon_gatts_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_data);
void beacon_set_app_advertisement_data();
