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
#include "reg.h"
#include "dev/ioc.h"
#include "button-sensor.h"
#include "adc.h"
#include "i2c.h"
#include "driver.h"
#include "dimmer.h"
#include "plugz-adc.h"
#include "math.h"

#define TRIAC_GPIO_BASE             GPIO_C_BASE
#define TRIAC_GPIO_PORT_NUM         GPIO_C_NUM
#define TRIAC1_GPIO_PIN             0
#define TRIAC2_GPIO_PIN             1
#define TRIAC3_GPIO_PIN             2
#define TRIAC4_GPIO_PIN             3
#define TRIAC_GPIO_PIN_MASK         0b1111

#define CURRENT_SENSOR_GPIO_BASE       GPIO_A_BASE
#define CURRENT_SENSOR_PORT_NUM        GPIO_A_NUM
#define CURRENT_SENSOR_GPIO_PIN        2
#define CURRENT_SENSOR_GPIO_PIN_MASK   0b100

#define TMP75_I2C_ID                0x48
#define TMP75_POINTER_REG           0
#define TMP75_TEMPERATURE_REG       0
#define TMP75_CONFIGURATION_REG     1

/* AC frequency - either 50hz or 60hz */
static uint8_t ac_frequency;
/* Time in microseconds for a full wave to complete. */
static uint32_t full_wave_ms;
/* Time in microseconds between each RT tick, here it is 30 usec */
static uint32_t rt_time_ms;

/*
 * Turn on given triac.
 */
void
plugz_triac_turn_on(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0xFF;
}

/*
 * Turn off given triac Triac.
 */
void
plugz_triac_turn_off(uint8_t triac_no)
{
   uint8_t address_bus_mask = (1 << triac_no) << 2;
   REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0;
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
 * Read raw current sensor value.
 */
static double
read_current_sensor_value()
{
   const double mv_per_amp = 18.5;
   double adc_value, result_mv, vdd, ref_mv;

   vdd = plugz_read_internal_voltage();
   ref_mv = vdd / 2;

   adc_value = adc_get(SOC_ADC_ADCCON_CH_AIN2, SOC_ADC_ADCCON_REF_AVDD5, SOC_ADC_ADCCON_DIV_512);
   result_mv = adc_to_volt(adc_value, vdd, adc_div_to_enob(SOC_ADC_ADCCON_DIV_512));

   return (result_mv - ref_mv) * mv_per_amp;
}

/*
 * Returns RMS of the AC current.
 *
 * Read current sensor value for a whole wavelength and then calculate RMS.
 */
double
plugz_read_current_sensor_value()
{
   int sample_count = 0;
   double v, sum = 0;
   rtimer_clock_t end;

   end = RTIMER_NOW() + (full_wave_ms / rt_time_ms);
   do {
      v = read_current_sensor_value();
      sum += (v * v);
      sample_count ++;
   } while(end > RTIMER_NOW());

   return sqrt(sum / sample_count);
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
 * Initialize the GPIO pins of the TRIACs.
 */
static inline void
triac_init()
{
   /* Configure TRIAC pins as output */
   GPIO_SOFTWARE_CONTROL(TRIAC_GPIO_BASE, TRIAC_GPIO_PIN_MASK);
   GPIO_SET_OUTPUT(TRIAC_GPIO_BASE, TRIAC_GPIO_PIN_MASK);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC1_GPIO_PIN, IOC_OVERRIDE_OE);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC2_GPIO_PIN, IOC_OVERRIDE_OE);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC3_GPIO_PIN, IOC_OVERRIDE_OE);
   ioc_set_over(TRIAC_GPIO_PORT_NUM, TRIAC4_GPIO_PIN, IOC_OVERRIDE_OE);
}

/*
 * Initialize current sensor(ACS716).
 */
static inline void
current_sensor_init()
{
   /* Configure current sensors as input */
   GPIO_SOFTWARE_CONTROL(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
   GPIO_SET_INPUT(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
   /* override the default pin configuration and set them as ANALOG */
   ioc_set_over(CURRENT_SENSOR_PORT_NUM, CURRENT_SENSOR_GPIO_PIN, IOC_OVERRIDE_ANA);
}

/*
 * Initialize temperature sensor(TMP75).
 */
static inline void
temperature_sensor_init()
{
#if USING_CC2538DK
   return;
#endif
   /* Configure the temperature sensor - TMP75 */
   i2c_smb_write_byte(TMP75_I2C_ID, TMP75_CONFIGURATION_REG, 0);
   i2c_smb_write_byte(TMP75_I2C_ID, TMP75_POINTER_REG, 0);
}

/*
 * Calculates the AC frequency using zero cross interrupt.
 */
static inline uint8_t
calculate_ac_frequency()
{
   /*TODO - actual calculation*/
   return 50;
}

/*
 * Initializes all GPIO pins and sets up required ISRs.
 */
void
plugz_switch_driver_init(void)
{
   triac_init();

   ac_frequency = calculate_ac_frequency();
   dimmer_init(ac_frequency);
   full_wave_ms = 1000000UL / ac_frequency;
   rt_time_ms = 1000000UL / RTIMER_ARCH_SECOND;

   current_sensor_init();

   button_init();
   adc_init();
   i2c_init();

   temperature_sensor_init();
}

/**
 * @}
 * @}
 */
