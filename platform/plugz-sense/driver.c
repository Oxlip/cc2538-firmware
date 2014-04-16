/**
 * \addtogroup plugz-usense
 * @{
 *
 * \defgroup plugz-usense driver
 *
 * PlugZ uSense driver
 * @{
 *
 * \file
 * uSense drivers
 */
#include "reg.h"
#include "dev/ioc.h"
#include "button-sensor.h"
#include "adc.h"
#include "i2c.h"
#include "driver.h"
#include "plugz-adc.h"

#define TMP75_I2C_ID                0x49
#define TMP75_POINTER_REG           0
#define TMP75_TEMPERATURE_REG       0
#define TMP75_CONFIGURATION_REG     1

static inline void
plugz_temperature_sensor_init()
{
   /* Configure the temperature sensor - TMP75 */
   i2c_smb_write_byte(TMP75_I2C_ID, TMP75_CONFIGURATION_REG, 0);
   i2c_smb_write_byte(TMP75_I2C_ID, TMP75_POINTER_REG, 0);
}

/*
 * Initializes all GPIO pins and sets up required ISRs.
 */
void
plugz_sense_driver_init(void)
{
   adc_init();

   i2c_init();

   plugz_temperature_sensor_init();
}

/*
 * Read onboard temperature sensor value.
 */
float
plugz_read_temperature_sensor_value()
{
   uint16_t digital_output = 0;
   const float celsius_factor = 0.0625;

   i2c_smb_read_word(TMP75_I2C_ID, TMP75_TEMPERATURE_REG, &digital_output);
   digital_output = digital_output >> 4;

   return digital_output * celsius_factor;
}

#ifdef ADC_DEBUG
/*
 * Read and prints given ADC.
 */
static void
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
/*   printf("milivolt = %-3d (raw %-6d ref_voltage %-4d resolution %-5d channel %s)\n",
          (int)adc_to_volt(adc_value, ref_voltage, enb),
          adc_value,
          (int)ref_voltage,
          1 << enb,
          adc_channel_str[ch]
         ); */
}
#endif

/**
 * @}
 * @}
 */

