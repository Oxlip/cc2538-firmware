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
#include <math.h>
#include "reg.h"
#include "dev/ioc.h"
#include "button-sensor.h"
#include "adc.h"
#include "i2c.h"
#include "driver.h"
#include "plugz-adc.h"

#define SI7013_I2C_ID                     0x40
#define SI7013_MEASURE_RH_CMD             0xE5
#define SI7013_MEASURE_TEMP_CMD           0xE3
#define SI7013_MEASURE_PREV_TEMP_CMD      0xE0

#define MAX44009_I2C_ID                   0x4A
#define MAX44009_INTR_STATUS_REG          0x0
#define MAX44009_INTR_ENABLE_REG          0x1
#define MAX44009_CONFIGURATION_REG        0x2
#define MAX44009_LUX_HIGH_REG             0x3
#define MAX44009_LUX_LOW_REG              0x4

#define MOTION_DETECTOR_GPIO_BASE         GPIO_C_BASE
#define MOTION_DETECTOR_GPIO_PIN          1
#define MOTION_DETECTOR_GPIO_PIN_MASK     (1 << MOTION_DETECTOR_GPIO_PIN)
#define MOTION_DETECTOR_PORT_NUM          GPIO_C_NUM
#define MOTION_DETECTOR_VECTOR            NVIC_INT_GPIO_PORT_C

static inline uint16_t
swap16(uint16_t word)
{
  return (word << 8) | (word >> 8);
}

/*
 * Reads values from Si7013 sensor and fills the temperature and humidity values.
 */
int
plugz_read_si7013(float *temperature, int32_t *humdity)
{
  uint16_t temp_code, rh_code;

  i2c_smb_read_word(SI7013_I2C_ID, SI7013_MEASURE_RH_CMD, &rh_code);
  rh_code = swap16(rh_code);
  i2c_smb_read_word(SI7013_I2C_ID, SI7013_MEASURE_PREV_TEMP_CMD, &temp_code);
  temp_code = swap16(temp_code);

  *humdity = ((rh_code * 15625) >> 13) - 6000;
  *temperature = ((temp_code * 21965) >> 13) - 46850;

  return 0;
}

/*
 * Convert Lux to percentage.
 */
float
plugz_lux_to_pct(float lux)
{
  const float lux_breakpoint = 1254.0;

  if (lux > lux_breakpoint) {
    return 1.0;
  } else {
    return (9.9323 * log(lux) + 27.059) / 100.0;
  }
}

/*
 * Read Ambient Light Sensor(MAX44009) value.
 */
float
plugz_read_ambient_lux()
{
  uint8_t exponent, mantissa, high, low;

  i2c_smb_read_byte(MAX44009_I2C_ID, MAX44009_LUX_HIGH_REG, &high);
  i2c_smb_read_byte(MAX44009_I2C_ID, MAX44009_LUX_LOW_REG, &low);

  exponent = (high & 0xF0) >> 4;
  mantissa = (high & 0x0F) << 4;
  mantissa |= (low & 0x0F);

  return mantissa * (1 << exponent) * 0.045f;
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
   /* Read cc2538 datasheet for internal ref voltage (1.19v) + vdd coefficient per v). + temp coefficient*/
   const double int_ref_mv = 1190;// 1190 + (3 * 2) + (30 / 10 * 0.4);

   adc_value = adc_get(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   pa_mv = adc_to_volt(adc_value, int_ref_mv, adc_div_to_enob(SOC_ADC_ADCCON_DIV_512));
   return pa_mv * 3;
}

/*
 * \brief Motion detected ISR callback.
 *
 * When the motion sensor detects motion it will generate an interrupt.
 *
 * \param port  The port number that generated the interrupt.
 * \param pin   The pin number that generated the interrupt. This is the pin
 *              absolute number (i.e. 0, 1, ..., 7), not a mask.
 */
static void
motion_detected_handler(uint8_t port, uint8_t pin)
{
  printf("Motion detected\n");
}

/*
 * Setup the motion sensor to generate an interrupt.
 * Also setup the micro-controller to wakeup when motion sensor generates an
 * interrupt.
 */
static inline void
motion_sensor_init()
{
  /* Configure Zero Cross pin as input */
  GPIO_SOFTWARE_CONTROL(MOTION_DETECTOR_GPIO_BASE, MOTION_DETECTOR_GPIO_PIN_MASK);
  GPIO_SET_INPUT(MOTION_DETECTOR_GPIO_BASE, MOTION_DETECTOR_GPIO_PIN_MASK);

  /* Trigger interrupt on falling edge */
  GPIO_DETECT_EDGE(MOTION_DETECTOR_GPIO_BASE, MOTION_DETECTOR_GPIO_PIN_MASK);
  GPIO_TRIGGER_SINGLE_EDGE(MOTION_DETECTOR_GPIO_BASE, MOTION_DETECTOR_GPIO_PIN_MASK);
  GPIO_DETECT_RISING(MOTION_DETECTOR_GPIO_BASE, MOTION_DETECTOR_GPIO_PIN_MASK);
  GPIO_ENABLE_INTERRUPT(MOTION_DETECTOR_GPIO_BASE, MOTION_DETECTOR_GPIO_PIN_MASK);

  gpio_register_callback(motion_detected_handler, MOTION_DETECTOR_PORT_NUM, MOTION_DETECTOR_GPIO_PIN);

  /* TODO generate interrupt. */
}

/*
 * Initializes all GPIO pins and sets up required ISRs.
 */
void
plugz_sense_driver_init(void)
{
  adc_init();
  i2c_init();
  motion_sensor_init();
}

/**
 * @}
 * @}
 */
