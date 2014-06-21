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

#define TMP75_I2C_ID                0x48
#define TMP75_POINTER_REG           0
#define TMP75_TEMPERATURE_REG       0
#define TMP75_CONFIGURATION_REG     1

static inline void
plugz_temperature_sensor_init()
{
#if USING_CC2538DK
   return;
#endif
   /* Configure the temperature sensor - TMP75 */
   i2c_smb_write_byte(TMP75_I2C_ID, TMP75_CONFIGURATION_REG, 0);
   i2c_smb_write_byte(TMP75_I2C_ID, TMP75_POINTER_REG, 0);
}

/*
 * Read temperature sensor value.
 */
float
plugz_read_temperature_sensor_value()
{
   uint16_t digital_output = 0;
   const float celsius_factor = 0.0625;

   i2c_smb_read_word(TMP75_I2C_ID, TMP75_TEMPERATURE_REG, &digital_output);
   digital_output = (digital_output << 8) | (digital_output >> 8);

   return (digital_output >> 4) * celsius_factor;
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

/**
 * @}
 * @}
 */

