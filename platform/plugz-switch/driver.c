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
#include "i2c.h"
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

/*
 * Initializes all GPIO pins and sets up required ISRs.
 */
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

/*
 * Turn on a Triac.
 */
void
plugz_triac_turn_on(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0xFF;
}

/*
 * Turn off a Triac.
 */
void
plugz_triac_turn_off(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0;
}

/*
 * Read onboard temperature sensor value.
 */
float
plugz_read_temperature_sensor_value()
{
   uint8_t high, low;

   high = i2c_read_byte(TMP75_I2C_ID);
   low = i2c_read_byte(TMP75_I2C_ID);

   return ((high << 8) | low) >> 4;
}

/*
 * Converts given adc_value to milivolt sensed at the ADC pin.
 *  ref_voltate - Reference voltagae used.
 *  enob - Effective number of bits.
 */
static inline float
adc_to_volt(int adc_value, float ref_voltage, int enob)
{
   const float resolution = (1 << enob) - 1;
   const float volts_per_bit = (ref_voltage / resolution) * 1000;

   return (float)adc_value * volts_per_bit;
}

/*
 * Converts given DIV to ENOB.
 */
static uint8_t
adc_div_to_enob(uint8_t div)
{
   switch (div) {
      case SOC_ADC_ADCCON_DIV_512:
         return 13;
      case SOC_ADC_ADCCON_DIV_256:
         return 10;
      case SOC_ADC_ADCCON_DIV_128:
         return 9;
      case SOC_ADC_ADCCON_DIV_64:
         return 7;
   }
   return -1;
}

/*
 * Read a single ADC channel's value.
 */
static int16_t
plugz_adc_read(uint8_t channel, uint8_t ref, uint8_t div)
{
   int16_t adc_value, result, is_negative, mask, upper_bits;
   uint8_t enb;

   enb = adc_div_to_enob(div);

   adc_value = adc_get(channel, ref, div) >> 2;

   upper_bits = 16 - enb;
   mask = (1 << upper_bits) - 1;
   is_negative = adc_value & (1 << 15);
   if (is_negative) {
      result = adc_value | (mask << enb);
   } else {
      result = adc_value & ((1 << enb) - 1);
   }

   return result;
}

/*
 * Read current sensor value.
 */
float
plugz_read_current_sensor_value()
{
   float adc_value;

   adc_value = plugz_adc_read(SOC_ADC_ADCCON_CH_AIN2, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   return adc_to_volt(adc_value, 3.3, adc_div_to_enob(SOC_ADC_ADCCON_DIV_512));
}

/*
 * Read and prints given ADC.
 */
void
print_adc_value(int16_t ch, int16_t ref, int16_t div)
{
   static char * adc_channel_str[] = {
               "PA0",
               "PA1",
               "PA2",
               "PA3",
               "PA4",
               "PA5",
               "PA6",
               "PA7",
               "PA0_PA1",
               "PA2_PA3",
               "PA4_PA5",
               "PA6_PA7",
               "GND",
               "RESERVED",
               "TEMP",
               "VDD_3",
               ""
   };

   int16_t adc_value, enb = adc_div_to_enob(div);
   float ref_voltage = 0;
   switch (ref) {
      case SOC_ADC_ADCCON_REF_AVDD5:
         ref_voltage = 3.3;
         break;
      case SOC_ADC_ADCCON_REF_INT:
         ref_voltage = 1.2;
         break;
   }

   adc_value = plugz_adc_read(ch, ref, div);
   printf("milivolt = %-3d (raw %-6d ref_voltage %-4d resolution %-5d channel %s)\n",
          (int)adc_to_volt(adc_value, ref_voltage, enb),
          adc_value,
          (int)ref_voltage,
          1 << enb,
          adc_channel_str[ch]
         );
}

/*
 * Debug helper - Prints all ADC values.
 */
void
plugz_print_adc_values()
{
#if 0
   print_adc_value(SOC_ADC_ADCCON_CH_AIN0, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_AIN1, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_AIN2, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_AIN3, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   printf("-\n");
   print_adc_value(SOC_ADC_ADCCON_CH_AIN0, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_AIN1, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_AIN2, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_AIN3, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   printf("-\n");

   print_adc_value(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_GND, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_TEMP, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   printf("-\n");
   print_adc_value(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_GND, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   print_adc_value(SOC_ADC_ADCCON_CH_TEMP, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   printf("-\n");
   print_adc_value(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_256);
   print_adc_value(SOC_ADC_ADCCON_CH_GND, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_256);
   print_adc_value(SOC_ADC_ADCCON_CH_TEMP, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_256);
   printf("-\n");
   print_adc_value(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_256);
   print_adc_value(SOC_ADC_ADCCON_CH_GND, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_256);
   print_adc_value(SOC_ADC_ADCCON_CH_TEMP, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_256);
#endif
}

/**
 * @}
 * @}
 */

