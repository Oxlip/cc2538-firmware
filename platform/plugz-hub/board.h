/** \addtogroup plugz-hub
 * @{
 *
 * \defgroup plugz-hub Devices
 *
 * Defines related to the Plugz-Hub
 *
 * This file provides connectivity information on UART and other pin information
 * on Plugz-Hub
 *
 *
 * \file
 * Header file with definitions related to the I/O connections on the Plugz-Hub
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"

/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PC0 and is shared with LED1
 */
#define USB_PULLUP_PORT          GPIO_C_BASE
#define USB_PULLUP_PIN           0
#define USB_PULLUP_PIN_MASK      (1 << USB_PULLUP_PIN)
/** @} */
 
/*---------------------------------------------------------------------------*/
/** \name UART configuration
 *
 * On the SmartRF06EB, the UART (XDS back channel) is connected to the
 * following ports/pins
 * - RX:  PA0
 * - TX:  PA1
 *
 * We configure the port to use UART0. To use UART1, change UART_CONF_BASE
 * @{
 */
#define UART_CONF_BASE           UART_0_BASE

#define UART_RX_PORT             GPIO_A_NUM
#define UART_RX_PIN              0

#define UART_TX_PORT             GPIO_A_NUM
#define UART_TX_PIN              1

#define UART_CTS_PORT            GPIO_B_NUM
#define UART_CTS_PIN             0

#define UART_RTS_PORT            GPIO_D_NUM
#define UART_RTS_PIN             3
/** @} */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "Plugz Hub"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */