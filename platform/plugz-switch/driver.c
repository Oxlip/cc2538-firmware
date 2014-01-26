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
#include "dev/ioc.h"
#include "button-sensor.h"
#include "adc.h"
#include "driver.h"
#include "dimmer.h"

#include <stdio.h>

#define TRIAC_GPIO_BASE             GPIO_C_BASE
#define TRIAC_GPIO_PORT_NUM         GPIO_C_NUM
#define TRIAC1_GPIO_PIN             0
#define TRIAC2_GPIO_PIN             1
#define TRIAC3_GPIO_PIN             2
#define TRIAC4_GPIO_PIN             3
#define TRIAC_GPIO_PIN_MASK         0b1111

#define CURRENT_SENSOR_GPIO_BASE    GPIO_A_BASE
#define CURRENT_SENSOR_PORT_NUM     GPIO_A_NUM
#define CURRENT_SENSOR_GPIO_PIN     2
#define CURRENT_SENSOR_GPIO_PIN_MASK   0b100

#define TMP75_I2C_ID                0x49
#define TMP75_POINTER_REG           0
#define TMP75_CONFIGURATION_REG     1

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

   dimmer_init();

   /* Configure current sensors as input */
   GPIO_SOFTWARE_CONTROL(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
   GPIO_SET_INPUT(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
   /* override the default pin configuration and set them as ANALOG */
   ioc_set_over(CURRENT_SENSOR_PORT_NUM, CURRENT_SENSOR_GPIO_PIN, IOC_OVERRIDE_ANA);

   button_init();

   adc_init();

   i2c_init();
   /* Configure the temperature sensor - TMP75 */
   i2c_write_byte(TMP75_CONFIGURATION_REG, TMP75_I2C_ID);
   i2c_write_byte(0, TMP75_I2C_ID); //default configuration
   i2c_write_byte(TMP75_POINTER_REG, TMP75_I2C_ID);
   i2c_write_byte(0, TMP75_I2C_ID);
}

/*---------------------------------------------------------------------------*/
void
plugz_triac_turn_on(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0xFF;
}

void
plugz_triac_turn_off(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0;
}

uint16_t
plugz_read_current_sensor_value()
{
   return adc_get(SOC_ADC_ADCCON_CH_AIN2, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
}

uint16_t
plugz_read_temperature_sensor_value()
{
   int temperature;
   uint8_t high, low;

   high = i2c_read_byte(TMP75_I2C_ID);
   low = i2c_read_byte(TMP75_I2C_ID);

   return ((high << 8) | low) >> 4;
}

/**
 * @}
 * @}
 */

