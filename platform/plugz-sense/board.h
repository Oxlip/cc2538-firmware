/** \addtogroup plugz-usense
 * @{
 *
 * \defgroup usense Devices
 *
 * Defines related to the plugz-sense
 *
 * This file provides connectivity information on UART and other pin information
 * on plugz-sense
 *
 *
 * \file
 * Header file with definitions related to the I/O connections on the plugz-sense
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"

#define USING_CC2538DK 1

/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PC0.
 */
#define USB_PULLUP_PORT          GPIO_C_BASE
#define USB_PULLUP_PIN           0

/*---------------------------------------------------------------------------*/
/** \name UART configuration
 *
 * On the SmartRF06EB, the UART (XDS back channel) is connected to the
 * following ports/pins
 * - RX:  PA0
 * - TX:  PA1
 *
 * We configure the port to use UART0. To use UART1, change UART_CONF_BASE
 */
#define UART_CONF_BASE           UART_0_BASE

#define UART0_RX_PORT             GPIO_A_NUM
#define UART0_RX_PIN              0

#define UART0_TX_PORT             GPIO_A_NUM
#define UART0_TX_PIN              1

/*---------------------------------------------------------------------------*/
/**
 * \name I2C Configuration
 */
#define I2C_CONF_SCL_PORT        GPIO_D_NUM
#define I2C_CONF_SCL_PIN         0
#define I2C_CONF_SDA_PORT        GPIO_D_NUM
#define I2C_CONF_SDA_PIN         1

/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "Plugz uSense"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */