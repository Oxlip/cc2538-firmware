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

#define SI7013_I2C_ID               0x40
#define SI7013_MEASURE_RH_CMD       0xE5
#define SI7013_MEASURE_TEMP_CMD     0xE3

#define MAX44009_I2C_ID             0x4A
#define MAX44009_INTR_STATUS_REG    0x0
#define MAX44009_INTR_ENABLE_REG    0x1
#define MAX44009_CONFIGURATION_REG  0x2
#define MAX44009_LUX_HIGH_REG       0x3
#define MAX44009_LUX_LOW_REG        0x4

/*
 * Read temperature sensor(Si7013) value.
 */
double
plugz_read_temperature()
{
  uint16_t temp_code;

  i2c_smb_read_word(SI7013_I2C_ID, SI7013_MEASURE_TEMP_CMD, &temp_code);
  return (double)((175.72f * temp_code) / 65536) - 46.85f;
}

/*
 * Read relative humdity sensor(Si7013) value.
 */
uint8_t
plugz_read_humidity()
{
  uint16_t rh_code;

  i2c_smb_read_word(SI7013_I2C_ID, SI7013_MEASURE_RH_CMD, &rh_code);
  return ((125 * rh_code) / 65536) - 6;
}

/*
 * Read Ambient Light Sensor(MAX44009) value.
 */
double
plugz_read_ambient_lux()
{
  uint16_t exponent = 0, mantissa = 0;
  uint8_t high, low;
  uint8_t exp_bits, mant_bits;
  int i;

  i2c_smb_read_byte(MAX44009_I2C_ID, MAX44009_LUX_HIGH_REG, &high);
  i2c_smb_read_byte(MAX44009_I2C_ID, MAX44009_LUX_HIGH_REG, &low);

  exp_bits = high >> 4;
  for(i = 0; i < 4; i++) {
    const int two_pow = 1 << i;
    exponent += (exp_bits & two_pow);
  }

  mant_bits = (high << 4) | (low & 0b1111);
  for(i = 0; i < 8; i++) {
    const int two_pow = 1 << i;
    mantissa += (mant_bits & two_pow);
  }

  return (1 << exponent) * mantissa * 0.72f;
}

/*
 * Reads Vdd supplied to CC2538.
 *
 * By using cc2538's internal voltage reference(1.19v) as reference voltage and
 * internal channel VDD/3, we can get the Vcc.
 *
 * Result is returned in milivolt units.
 */
double
plugz_read_internal_voltage()
{
   int16_t adc_value;
   double pa_mv;
   /* Read cc2538 datasheet for internal ref voltation(1.19v) + vdd coeffient(2mv per v). + temp coefficent*/
   const double int_ref_mv = 1190;// 1190 + (3 * 2) + (30 / 10 * 0.4);

   adc_value = adc_get(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   pa_mv = adc_to_volt(adc_value, int_ref_mv, adc_div_to_enob(SOC_ADC_ADCCON_DIV_512));
   return pa_mv * 3;
}

/*
 * Initializes all GPIO pins and sets up required ISRs.
 */
void
plugz_sense_driver_init(void)
{
  adc_init();
  i2c_init();
}

/**
 * @}
 * @}
 */
