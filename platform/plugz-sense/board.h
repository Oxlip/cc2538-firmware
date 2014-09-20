/** \addtogroup uSense
 * @{
 *
 * \defgroup uSense Devices
 *
 * Defines related to the uSense
 *
 * This file provides connectivity information on UART and other pin information
 * on uSense
 *
 *
 * \file
 * Header file with definitions related to the I/O connections on the uSense
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"

#define USING_CC2538DK          1
/*---------------------------------------------------------------------------*/
/** \name SmartRF LED configuration
 *
 * LEDs on the SmartRF06 (EB and BB) are connected as follows:
 * - LED1 (Red)    -> PC0
 * - LED2 (Yellow) -> PC1
 * - LED3 (Green)  -> PC2
 * - LED4 (Orange) -> PC3
 *
 * LED1 shares the same pin with the USB pullup
 * @{
 */
/*---------------------------------------------------------------------------*/
/* Some files include leds.h before us, so we need to get rid of defaults in
 * leds.h before we provide correct definitions */
#undef LEDS_GREEN
#undef LEDS_YELLOW
#undef LEDS_RED
#undef LEDS_CONF_ALL

#define LEDS_YELLOW    2 /**< LED2 (Yellow) -> PC1 */
#define LEDS_GREEN     4 /**< LED3 (Green)  -> PC2 */
#define LEDS_ORANGE    8 /**< LED4 (Orange) -> PC3 */

#if USB_SERIAL_CONF_ENABLE
#define LEDS_CONF_ALL 14
#define LEDS_RED LEDS_ORANGE
#else
#define LEDS_CONF_ALL 15
#define LEDS_RED       1 /**< LED1 (Red)    -> PC0 */
#endif

/* Notify various examples that we have LEDs */
#define PLATFORM_HAS_LEDS        1
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
#define UART_CONF_BASE            UART_0_BASE

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
#define BOARD_STRING "uSense"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */
