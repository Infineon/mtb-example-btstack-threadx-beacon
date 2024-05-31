/*
 * $ Copyright YEAR Cypress Semiconductor $
 */

/** @file
*
* Beacon sample code with BTSTACK v3 APIs
*
*/
#include "wiced_bt_gatt.h"
#include "wiced_memory.h"
#include "cycfg_gap.h"
#include "cycfg_gatt_db.h"
#include "beacon.h"
#include "stdlib.h"
#include "stdio.h"

extern const wiced_bt_cfg_settings_t app_cfg_settings;
typedef void (*pfn_free_buffer_t)(uint8_t *);

/******************************************************************************
 *                             Local Function Definitions
 ******************************************************************************/
void* app_alloc_buffer(int len)
{
    uint8_t *p = (uint8_t *)malloc(len);
    printf( "%s() len %d alloc %p \n", __FUNCTION__,len, p);
    return p;
}

void app_free_buffer(uint8_t *p_buf)
{
    if (p_buf != NULL)
    {
        printf( "%s()free:%p \n",__FUNCTION__, p_buf);
        free(p_buf);
    }
}

/******************************************************************************
 *                          Function Definitions
 ******************************************************************************/

/*
 * Setup advertisement data with configured advertisment array from BTConfigurator
 * By default, includes 16 byte UUID and device name
 */
void beacon_set_app_advertisement_data(void)
{
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);
}

/*
 * Process Read request from peer device
 */
wiced_bt_gatt_status_t beacon_req_read_handler(uint16_t conn_id,
        wiced_bt_gatt_opcode_t opcode,
        wiced_bt_gatt_read_t *p_read_req,
        uint16_t len_requested)
{
    int to_copy;
    uint8_t * copy_from;

    switch(p_read_req->handle)
    {
    case HDLC_GAP_DEVICE_NAME_VALUE:
        to_copy = app_gap_device_name_len;
        copy_from = (uint8_t *) app_gap_device_name;
        break;

    case HDLC_GAP_APPEARANCE_VALUE:
        to_copy = 2;
        copy_from = (uint8_t *) &app_cfg_settings.p_ble_cfg->appearance;
        break;

    default:
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    // Adjust copying location & length limit based on offset
    copy_from += p_read_req->offset;
    to_copy -= p_read_req->offset;

    // if copy length is not valid, then that means the offset of over the limit
    if (to_copy <= 0)
    {
        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_read_req->handle,
            WICED_BT_GATT_INVALID_OFFSET);
        return WICED_BT_GATT_INVALID_OFFSET;
    }

    // If the copying length is not specified or if it is over the size, use the max can copy
    if (len_requested == 0 || len_requested > to_copy)
    {
        len_requested = to_copy;
    }

    wiced_bt_gatt_server_send_read_handle_rsp(conn_id, opcode, len_requested, copy_from, NULL);

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process Read by type request from peer device
 */
wiced_bt_gatt_status_t beacon_req_read_by_type_handler(uint16_t conn_id,
        wiced_bt_gatt_opcode_t opcode,
        wiced_bt_gatt_read_by_type_t *p_read_req,
        uint16_t len_requested)
{
    int         to_copy;
    uint8_t     * copy_from;
    uint16_t    attr_handle = p_read_req->s_handle;
    uint8_t    *p_rsp = wiced_bt_get_buffer(len_requested);
    uint8_t     pair_len = 0;
    int used = 0;

    if (p_rsp == NULL)
    {
        printf("[%s] no memory len_requested: %d!!\n", __FUNCTION__,
                len_requested);
        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, attr_handle,
                WICED_BT_GATT_INSUF_RESOURCE);
        return WICED_BT_GATT_INSUF_RESOURCE;
    }

    /* Read by type returns all attributes of the specified type, between the start and end handles */
    while (WICED_TRUE)
    {
        /// Add your code here
        attr_handle = wiced_bt_gatt_find_handle_by_type(attr_handle,
                p_read_req->e_handle, &p_read_req->uuid);

        if (attr_handle == 0)
            break;

        switch(attr_handle)
        {
        case HDLC_GAP_DEVICE_NAME_VALUE:
            to_copy = app_gap_device_name_len;
            copy_from = (uint8_t *) app_gap_device_name;
            break;

        case HDLC_GAP_APPEARANCE_VALUE:
            to_copy = 2;
            copy_from = (uint8_t *) &app_cfg_settings.p_ble_cfg->appearance;
            break;

        default:
            printf("[%s] found type but no attribute ??\n", __FUNCTION__);
            wiced_bt_gatt_server_send_error_rsp(conn_id, opcode,
                    p_read_req->s_handle, WICED_BT_GATT_ERR_UNLIKELY);
            wiced_bt_free_buffer(p_rsp);
            return WICED_BT_GATT_ERR_UNLIKELY;
        }

        int filled = wiced_bt_gatt_put_read_by_type_rsp_in_stream(
                p_rsp + used,
                len_requested - used,
                &pair_len,
                attr_handle,
                to_copy,
                copy_from);

        if (filled == 0) {
            break;
        }
        used += filled;

        /* Increment starting handle for next search to one past current */
        attr_handle++;
    }

    if (used == 0)
    {
        printf("[%s] attr not found 0x%04x -  0x%04x Type: 0x%04x\n",
                __FUNCTION__, p_read_req->s_handle, p_read_req->e_handle,
                p_read_req->uuid.uu.uuid16);

        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_read_req->s_handle,
                WICED_BT_GATT_INVALID_HANDLE);
        wiced_bt_free_buffer(p_rsp);
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    /* Send the response */
    wiced_bt_gatt_server_send_read_by_type_rsp(conn_id, opcode, pair_len,
            used, p_rsp, (wiced_bt_gatt_app_context_t)wiced_bt_free_buffer);

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process read multi request from peer device
 */
wiced_bt_gatt_status_t beacon_req_read_multi_handler(uint16_t conn_id,
        wiced_bt_gatt_opcode_t opcode,
        wiced_bt_gatt_read_multiple_req_t *p_read_req,
        uint16_t len_requested)
{
    uint8_t     *p_rsp = wiced_bt_get_buffer(len_requested);
    int         used = 0;
    int         xx;
    uint16_t    handle;
    int         to_copy;
    uint8_t     * copy_from;

    handle = wiced_bt_gatt_get_handle_from_stream(p_read_req->p_handle_stream, 0);

    if (p_rsp == NULL)
    {
        printf ("[%s] no memory len_requested: %d!!\n", __FUNCTION__,
                len_requested);

        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, handle,
                WICED_BT_GATT_INSUF_RESOURCE);
        return WICED_BT_GATT_INSUF_RESOURCE;
    }

    /* Read by type returns all attributes of the specified type, between the start and end handles */
    for (xx = 0; xx < p_read_req->num_handles; xx++)
    {
        handle = wiced_bt_gatt_get_handle_from_stream(p_read_req->p_handle_stream, xx);

        switch(handle)
        {
        case HDLC_GAP_DEVICE_NAME_VALUE:
            to_copy = app_gap_device_name_len;
            copy_from = (uint8_t *) app_gap_device_name;
            break;

        case HDLC_GAP_APPEARANCE_VALUE:
            to_copy = 2;
            copy_from = (uint8_t *) &app_cfg_settings.p_ble_cfg->appearance;
            break;

        default:
            printf ("[%s] no handle 0x%04xn", __FUNCTION__, handle);
            wiced_bt_gatt_server_send_error_rsp(conn_id, opcode,
                    *p_read_req->p_handle_stream, WICED_BT_GATT_ERR_UNLIKELY);
            wiced_bt_free_buffer(p_rsp);
            return WICED_BT_GATT_ERR_UNLIKELY;
        }
        int filled = wiced_bt_gatt_put_read_multi_rsp_in_stream(opcode,
                    p_rsp + used,
                    len_requested - used,
                    handle,
                    to_copy,
                    copy_from);

        if (!filled) {
            break;
        }
        used += filled;
    }

    if (used == 0)
    {
        printf ("[%s] no attr found\n", __FUNCTION__);

        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode,
                *p_read_req->p_handle_stream, WICED_BT_GATT_INVALID_HANDLE);
        wiced_bt_free_buffer(p_rsp);
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    /* Send the response */
    wiced_bt_gatt_server_send_read_multiple_rsp(conn_id, opcode, used, p_rsp,
            (wiced_bt_gatt_app_context_t)wiced_bt_free_buffer);

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process write request or write command from peer device
 */
wiced_bt_gatt_status_t beacon_write_handler(uint16_t conn_id,
        wiced_bt_gatt_opcode_t opcode,
        wiced_bt_gatt_write_req_t* p_data)
{
    printf("[%s] conn_id:%d handle:%04x\n", __FUNCTION__, conn_id,
            p_data->handle);

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process MTU request from the peer
 */
wiced_bt_gatt_status_t beacon_req_mtu_handler( uint16_t conn_id, uint16_t mtu)
{
    printf("req_mtu: %d\n", mtu);
    wiced_bt_gatt_server_send_mtu_rsp(conn_id, mtu,
            app_cfg_settings.p_ble_cfg->ble_max_rx_pdu_size);
    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process indication confirm.
 */
wiced_bt_gatt_status_t beacon_req_value_conf_handler(uint16_t conn_id,
        uint16_t handle)
{
    printf("[%s] conn_id:%d handle:%x\n", __FUNCTION__, conn_id, handle);

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Process GATT request from the peer
 */
wiced_bt_gatt_status_t beacon_gatts_req_callback(wiced_bt_gatt_attribute_request_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_INVALID_PDU;

    switch (p_data->opcode)
    {
        case GATT_REQ_READ:
        case GATT_REQ_READ_BLOB:
            result = beacon_req_read_handler(p_data->conn_id,
                    p_data->opcode,
                    &p_data->data.read_req,
                    p_data->len_requested);
            break;

        case GATT_REQ_READ_BY_TYPE:
            result = beacon_req_read_by_type_handler(p_data->conn_id,
                    p_data->opcode,
                    &p_data->data.read_by_type,
                    p_data->len_requested);
            break;

        case GATT_REQ_READ_MULTI:
        case GATT_REQ_READ_MULTI_VAR_LENGTH:
            result = beacon_req_read_multi_handler(p_data->conn_id,
                    p_data->opcode,
                    &p_data->data.read_multiple_req,
                    p_data->len_requested);
            break;

        case GATT_REQ_WRITE:
        case GATT_CMD_WRITE:
        case GATT_CMD_SIGNED_WRITE:
            result = beacon_write_handler(p_data->conn_id,
                    p_data->opcode,
                    &(p_data->data.write_req));
            if (result == WICED_BT_GATT_SUCCESS)
            {
                wiced_bt_gatt_server_send_write_rsp(
                        p_data->conn_id,
                        p_data->opcode,
                        p_data->data.write_req.handle);
            }
            else
            {
                wiced_bt_gatt_server_send_error_rsp(
                        p_data->conn_id,
                        p_data->opcode,
                        p_data->data.write_req.handle,
                        result);
            }
            break;

        case GATT_REQ_MTU:
            result = beacon_req_mtu_handler(p_data->conn_id,
                    p_data->data.remote_mtu);
            break;

        case GATT_HANDLE_VALUE_CONF:
            result = beacon_req_value_conf_handler(p_data->conn_id,
                    p_data->data.confirm.handle);
            break;

       default:
            printf("Invalid GATT request conn_id:%d opcode:%d\n",
                    p_data->conn_id, p_data->opcode);
            break;
    }

    return result;
}

/*
 * Callback for various GATT events.  As this application performs only as a GATT server, some of the events are ommitted.
 */
wiced_bt_gatt_status_t beacon_gatts_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_INVALID_PDU;

    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        result = beacon_connection_status_event(&p_data->connection_status);
        break;

    case GATT_ATTRIBUTE_REQUEST_EVT:
        result = beacon_gatts_req_callback(&p_data->attribute_request);
        break;

    case GATT_GET_RESPONSE_BUFFER_EVT:
        p_data->buffer_request.buffer.p_app_rsp_buffer = app_alloc_buffer (p_data->buffer_request.len_requested);
        p_data->buffer_request.buffer.p_app_ctxt       = (void *)app_free_buffer;
        result = WICED_BT_GATT_SUCCESS;
        break;

    case GATT_APP_BUFFER_TRANSMITTED_EVT:
        {
            pfn_free_buffer_t pfn_free = (pfn_free_buffer_t)p_data->buffer_xmitted.p_app_ctxt;

            /* If the buffer is dynamic, the context will point to a function to free it. */
            if (pfn_free)
                pfn_free(p_data->buffer_xmitted.p_app_data);

            result = WICED_BT_GATT_SUCCESS;
        }
        break;
    default:
        break;
    }
    return result;
}
