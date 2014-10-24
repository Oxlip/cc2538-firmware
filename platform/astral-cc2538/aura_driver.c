/**
 * \addtogroup Astral
 * @{
 *
 * \defgroup Astral Aura Driver
 *
 * @{
 *
 * \file Driver for Astral Aura and Norma devices.
 */
#include "reg.h"
#include "dev/ioc.h"
#include "button-sensor.h"
#include "adc.h"
#include "i2c.h"
#include "aura_driver.h"
#include "math.h"

#define TRIAC_GPIO_BASE             GPIO_C_BASE
#define TRIAC_GPIO_PORT_NUM         GPIO_C_NUM
#define TRIAC1_GPIO_PIN             0
#define TRIAC2_GPIO_PIN             1
#define TRIAC3_GPIO_PIN             2
#define TRIAC4_GPIO_PIN             3
#define TRIAC_GPIO_PIN_MASK         0b1111

#define TMP75_I2C_ID                0x48
#define TMP75_POINTER_REG           0
#define TMP75_TEMPERATURE_REG       0
#define TMP75_CONFIGURATION_REG     1

/* Constants for the Maxim current sensor 78M6610+LMU */
#define CS_I2C_ID                   0x2
#define CS_REG_VA_RMS               0x2B
#define CS_REG_IA_RMS               0x3E
#define CS_REG_WATT_ACTIVE          0x4B
#define CS_REG_VA_PEAK              0x3A
#define CS_REG_IA_PEAK              0x46
#define CS_REG_VA                   0x33
#define CS_REG_IA                   0x44

/* AC frequency - either 50hz or 60hz */
static uint8_t ac_frequency;
/* Time in microseconds for a full wave to complete. */
static uint32_t full_wave_ms;
/* Time in microseconds between each RT tick, here it is 30 usec */
static uint32_t rt_time_ms;

/*
 * Returns current sensor value.
 */
double
get_cs_value(CS_VALUE_TYPE type, uint8_t input)
{
   uint32_t result;
   uint16_t reg;

   switch(type) {
      case CS_VALUE_TYPE_RMS_CURRENT:
         reg = CS_REG_IA_RMS;
         break;
      case CS_VALUE_TYPE_RMS_VOLTAGE:
         reg = CS_REG_VA_RMS;
         break;
      case CS_VALUE_TYPE_ACTIVE_WATT:
         reg = CS_REG_WATT_ACTIVE;
         break;
      case CS_VALUE_TYPE_VA_PEAK:
         reg = CS_REG_VA_PEAK;
         break;
      case CS_VALUE_TYPE_IA_PEAK:
         reg = CS_REG_IA_PEAK;
         break;
      case CS_VALUE_TYPE_VA:
         reg = CS_REG_VA;
         break;
      case CS_VALUE_TYPE_IA:
         reg = CS_REG_IA;
         break;
      default:
         reg = 0;
   }
   if (input >= 2) {
      reg ++;
   }
   i2c_smb_read_bytes(CS_I2C_ID, reg, &result, 3);

   /*TODO - This conversion may be wrong - read the datasheet*/
   return (double)result;
}

/*
 * Turn on/off the given triac.
 */
void
set_triac(uint8_t triac_no, uint8_t on)
{
   if (on) {
      uint8_t address_bus_mask = (1 << triac_no) << 2;
      REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0xFF;
   } else {
      uint8_t address_bus_mask = (1 << triac_no) << 2;
      REG((TRIAC_GPIO_BASE + GPIO_DATA) | address_bus_mask) = 0;
   }
}

/*
 * Read temperature sensor value.
 */
float
get_temperature()
{
   uint16_t digital_output = 0;
   const float celsius_factor = 0.0625;

   i2c_smb_read_word(TMP75_I2C_ID, TMP75_TEMPERATURE_REG, &digital_output);
   digital_output = (digital_output << 8) | (digital_output >> 8);

   return (digital_output >> 4) * celsius_factor;
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
 * Initialize current sensor(78M6610+LMU).
 */
static inline void
current_sensor_init()
{
}

/*
 * Initializes all GPIO pins and sets up required ISRs.
 */
void
driver_init(void)
{
   triac_init();

   ac_frequency = calculate_ac_frequency();
   dimmer_init(ac_frequency);
   full_wave_ms = 1000000UL / ac_frequency;
   rt_time_ms = 1000000UL / RTIMER_ARCH_SECOND;

   button_init();
   adc_init();
   i2c_init();
   
   current_sensor_init();

   temperature_sensor_init();
}

/**
 * @}
 * @}
 */
