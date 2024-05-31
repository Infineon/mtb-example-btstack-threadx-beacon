/*
 * Copyright 2016-2024, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/*
*  Utility used by beacon functions
*/

#include "wiced_bt_stack.h"
#include "wiced_bt_beacon.h"
#include "string.h"

/*
* This function copies advertiement elements into a flat buffer and return the lenght
*/
void wiced_bt_beacon_set_adv_data(wiced_bt_beacon_ble_advert_elem_t *beacon_adv_elem, uint8_t num_elem,
                                  uint8_t adv_data[WICED_BT_BEACON_ADV_DATA_MAX], uint8_t *adv_len)
{
    uint8_t i = 0, j = 0;
    for (i = 0; i < num_elem; i++)
    {
        adv_data[j++] = beacon_adv_elem[i].len;
        adv_data[j++] = beacon_adv_elem[i].advert_type;
        memcpy(&adv_data[j], beacon_adv_elem[i].data, (beacon_adv_elem[i].len - 1)); // len -1 for advert_type
        j += beacon_adv_elem[i].len - 1;
    }
    *adv_len = j;
}

// For 43012C0 platform, the multi adv APIs are not defined in ROM, hence define them in library
#if defined (CYW43012C0) && !defined(USE_CYW43012C0_MULTIADV_LIB)

#define BTM_SUCCESS                                          0
#define BTM_ILLEGAL_VALUE                                    5

#define HCIC_PARAM_SIZE_ENABLE_MULTI_ADV                     3  // sub-opcode + advertising_enable + adv_instance
#define HCIC_PARAM_SIZE_SET_MULTI_ADV_PARAM                  24 // sub-opcode + params
#define HCIC_PARAM_SIZE_SET_MULTI_ADV_MIN_SIZE               3  // sub-opcode + data len + adv instance + 0 size data
#define HCIC_PARAM_SIZE_SET_M ULTI_ADV_MAX_SIZE              34 // sub-opcode + data len + adv instance + Max data size

#define HCI_BRCM_ENABLE_MULTI_ADV                            (0x0154 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_BRCM_SET_MULTI_ADV_PARAMS                        (0x0154 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_BRCM_SET_MULTI_ADV_DATA                          (0x0154 | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_MULTI_ADVT_SUB_OCF_SET_ADVT_PARAM_MULTI          0x01
#define HCI_MULTI_ADVT_SUB_OCF_SET_ADVT_DATA_MULTI           0x02
#define HCI_MULTI_ADVT_SUB_OCF_SET_ADVT_ENABLE_MULTI         0x05

#define HCIC_PARAM_SIZE_SET_MULTI_ADV_MAX_SIZE               0x31
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA                   31

wiced_result_t wiced_start_multi_advertisements( uint8_t advertising_enable, uint8_t adv_instance )
{
    uint8_t param[HCIC_PARAM_SIZE_ENABLE_MULTI_ADV] = {0};
    uint8_t *pp = param;
    wiced_result_t status = BTM_SUCCESS;

    UINT8_TO_STREAM(pp, HCI_MULTI_ADVT_SUB_OCF_SET_ADVT_ENABLE_MULTI);
    UINT8_TO_STREAM(pp, advertising_enable);
    UINT8_TO_STREAM(pp, adv_instance);

    status = (wiced_result_t)wiced_bt_dev_vendor_specific_command ( HCI_BRCM_ENABLE_MULTI_ADV,
                                         HCIC_PARAM_SIZE_ENABLE_MULTI_ADV, param, NULL);

    return status;
}

wiced_result_t wiced_set_multi_advertisement_params( uint16_t advertising_interval_min,
                                                   uint16_t advertising_interval_max,
                                                   wiced_bt_ble_multi_advert_type_t advertising_type, wiced_bt_ble_address_type_t own_address_type,
                                                   wiced_bt_device_address_t ownAddr, wiced_bt_ble_address_type_t peerAddrType, wiced_bt_device_address_t peerAddr,
                                                   wiced_bt_ble_advert_chnl_map_t advertising_channel_map, wiced_bt_ble_multi_advert_filtering_policy_t advertising_filter_policy,
                                                   uint8_t adv_instance, int8_t  transmit_power )
{
    uint8_t param[HCIC_PARAM_SIZE_SET_MULTI_ADV_PARAM] = {0};
    uint8_t *pp = param;
    wiced_result_t status = BTM_SUCCESS;

    UINT8_TO_STREAM  (pp, HCI_MULTI_ADVT_SUB_OCF_SET_ADVT_PARAM_MULTI);
    UINT16_TO_STREAM  (pp, advertising_interval_min);
    UINT16_TO_STREAM  (pp, advertising_interval_max);
    UINT8_TO_STREAM  (pp, advertising_type);
    UINT8_TO_STREAM  (pp, own_address_type);
    BDADDR_TO_STREAM(pp, ownAddr);
    UINT8_TO_STREAM  (pp, peerAddrType);
    BDADDR_TO_STREAM (pp, peerAddr);
    UINT8_TO_STREAM  (pp, advertising_channel_map);
    UINT8_TO_STREAM  (pp, advertising_filter_policy);
    UINT8_TO_STREAM  (pp, adv_instance);
    INT8_TO_STREAM  (pp, transmit_power);

    status = (wiced_result_t)wiced_bt_dev_vendor_specific_command(HCI_BRCM_SET_MULTI_ADV_PARAMS,
        HCIC_PARAM_SIZE_SET_MULTI_ADV_PARAM, param, NULL);

    return status;
}

wiced_result_t wiced_set_multi_advertisement_data( uint8_t * p_data, uint8_t data_len, uint8_t adv_instance )
{
    uint8_t param[HCIC_PARAM_SIZE_SET_MULTI_ADV_MAX_SIZE] = {0};
    uint8_t *pp = param;
    wiced_result_t status = BTM_SUCCESS;
    uint8_t total_size = HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA + HCIC_PARAM_SIZE_SET_MULTI_ADV_MIN_SIZE;

    if (data_len > 31 || (p_data == NULL && data_len > 0))
    {
        return (uint8_t)BTM_ILLEGAL_VALUE;
    }

    UINT8_TO_STREAM  (pp, HCI_MULTI_ADVT_SUB_OCF_SET_ADVT_DATA_MULTI);

    memset(pp, 0, 1 + HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA); // data_len + data
    if (p_data != NULL && data_len > 0)
    {
        UINT8_TO_STREAM (pp, data_len);

        ARRAY_TO_STREAM (pp, p_data, data_len);
    }
    pp += (HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA - data_len);

    UINT8_TO_STREAM  (pp, adv_instance);

    status = (wiced_result_t)wiced_bt_dev_vendor_specific_command(HCI_BRCM_SET_MULTI_ADV_DATA, total_size, param, NULL);

    return status;
}
#endif
