/**
 * \addtogroup plugz-switch
 * @{
 *
 * \defgroup plugz-switch driver
 *
 * PlugZ Switch driver
 * @{
 *
 * \file
 * Triac, Current sensor, Temp sensor and Switch driver for PlugZ-Switch board
 */
#include "contiki.h"
#include "reg.h"
#include "dev/gpio.h"
#include "dev/ioc.h"
#include "button-sensor.h"
#include "adc.h"

#include <stdio.h>

#define TRIAC_GPIO_BASE             GPIO_C_BASE
#define TRIAC_GPIO_PORT_NUM         GPIO_C_NUM
#define TRIAC1_GPIO_PIN             0
#define TRIAC2_GPIO_PIN             1
#define TRIAC3_GPIO_PIN             2
#define TRIAC4_GPIO_PIN             3
#define TRIAC_GPIO_PIN_MASK         0b1111

#define ZERO_CROSS_GPIO_BASE        GPIO_C_BASE
#define ZERO_CROSS_GPIO_PIN         7
#define ZERO_CROSS_GPIO_PIN_MASK    (1 << ZERO_CROSS_GPIO_PIN)
#define ZERO_CROSS_PORT_NUM         GPIO_C_NUM
#define ZERO_CROSS_VECTOR           NVIC_INT_GPIO_PORT_C

#define CURRENT_SENSOR_GPIO_BASE    GPIO_A_BASE
#define CURRENT_SENSOR_PORT_NUM     GPIO_A_NUM
#define CURRENT_SENSOR_GPIO_PIN     2
#define CURRENT_SENSOR_GPIO_PIN_MASK   0b100

/*
 * \brief Callback registered with the Zero Cross detection.
 * \param port The port number that generated the interrupt
 * \param pin The pin number that generated the interrupt. This is the pin
 * absolute number (i.e. 0, 1, ..., 7), not a mask
 */
static void
zero_cross_detected(uint8_t port, uint8_t pin)
{
   extern void zero_cross_handler();
   zero_cross_handler();
}

/*---------------------------------------------------------------------------*/
void
plugz_switch_driver_init(void)
{
   /* Configure TRIAC pins as output */
   GPIO_SOFTWARE_CONTROL(TRIAC_GPIO_BASE, TRIAC_GPIO_PIN_MASK);
   GPIO_SET_OUTPUT(TRIAC_GPIO_BASE, TRIAC_GPIO_PIN_MASK);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC1_GPIO_PIN, IOC_OVERRIDE_OE);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC2_GPIO_PIN, IOC_OVERRIDE_OE);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC3_GPIO_PIN, IOC_OVERRIDE_OE);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC4_GPIO_PIN, IOC_OVERRIDE_OE);

   /* Configure Zero Cross pin as input */
   GPIO_SOFTWARE_CONTROL(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_SET_INPUT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

   /* Trigger interrupt on falling edge */
   GPIO_DETECT_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_TRIGGER_SINGLE_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_DETECT_RISING(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_ENABLE_INTERRUPT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

   //ioc_set_over(ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PINZERO_CROSS_GPIO_PIN, IOC_OVERRIDE_PUE);
   nvic_interrupt_enable(ZERO_CROSS_VECTOR);
   gpio_register_callback(zero_cross_detected, ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PIN);

   /* Configure current sensors as input */
   GPIO_SOFTWARE_CONTROL(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
   GPIO_SET_INPUT(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
   /* override the default pin configuration and set them as ANALOG */
   ioc_set_over(CURRENT_SENSOR_PORT_NUM, CURRENT_SENSOR_GPIO_PIN, IOC_OVERRIDE_ANA);

   button_init();

   adc_init();
}

/*---------------------------------------------------------------------------*/
void
plugz_triac_turn_on(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0xFF;
}

/*---------------------------------------------------------------------------*/
void
plugz_triac_turn_off(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
uint16_t
plugz_read_current_sensor_value()
{
   return adc_get(SOC_ADC_ADCCON_CH_AIN2, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */

