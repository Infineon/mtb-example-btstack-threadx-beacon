/*
 * $ Copyright YEAR Cypress Semiconductor $
 */

/** @file
*
* Beacon header file
*/
#ifndef _BEACON_H_
#define _BEACON_H_

#include "wiced_bt_gatt.h"

/*
 * Connection up/down event
 */
wiced_bt_gatt_status_t beacon_connection_status_event(wiced_bt_gatt_connection_status_t *p_status);


/*
 *  Entry point to the application. Set device configuration and start BT
 *  stack initialization.  The actual application initialization will happen
 *  when stack reports that BT device is ready.
 */
void application_start( void );

#endif // _BEACON_H_
