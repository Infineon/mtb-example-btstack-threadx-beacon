/*
 * $ Copyright YEAR Cypress Semiconductor $
 */

/** @file
*
* File Name:   main.c
*
* Description: This is the source code for the Bluetooth LE Beacon application
*              for ModusToolbox.
*
* Related Document: See README.md
*
*/
#include "stdio.h"
#include "cybsp.h"
#include "cyhal.h"
#include "string.h"
#include "wiced_bt_stack.h"
#include "beacon.h"
#include "cy_retarget_io.h"


int main()
{
    cy_rslt_t result;
    cy_rslt_t cy_result;

    /* Initialize the board support package */
    result = cybsp_init() ;

    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

        /* Initialize retarget-io to use the debug UART port */
    cy_result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (cy_result != CY_RSLT_SUCCESS)
    {
        printf("Retarget IO init failed\n");
        CY_ASSERT(0);
    }

    printf("EXTENDED ADVERTISEMENT BEACON APPLICATION \n");

    application_start();

}

/* [] END OF FILE */
