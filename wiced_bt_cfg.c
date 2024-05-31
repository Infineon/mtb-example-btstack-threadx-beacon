/*
 * $ Copyright YEAR Cypress Semiconductor $
 */

/** @file
 *
 * Runtime Bluetooth stack configuration parameters
 *
 */
#include <cycfg_bt_settings.h>
#include <cycfg_gap.h>
#include <cycfg_gatt_db.h>
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_cfg.h"

 /* wiced_bt core stack configuration */
const wiced_bt_cfg_settings_t app_cfg_settings =
{
    .device_name = (uint8_t*)cy_bt_device_name,            /**< Local device name (NULL terminated). Use same as configurator generated string.*/
    .security_required = CY_BT_SECURITY_LEVEL,               /**< Security requirements mask */
    .p_ble_cfg = &cy_bt_cfg_ble,
    .p_gatt_cfg = &cy_bt_cfg_gatt,
};
