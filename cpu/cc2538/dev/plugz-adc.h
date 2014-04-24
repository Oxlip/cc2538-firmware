/**
 * @{
 *
 * \defgroup plugz-switch ADC Driver
 *
 * ADC wrapper
 * @{
 *
 * \file
 * PlugZ specific ADC changes.
 */
#ifndef PLUGZ_ADC_H_
#define PLUGZ_ADC_H_
#include "driver.h"

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
static inline int16_t
plugz_adc_read(uint8_t channel, uint8_t ref, uint8_t div)
{
   int16_t adc_value, result, is_negative, mask, upper_bits;
   uint8_t enb;
   const uint8_t total_bits = 16;
   const uint16_t msb_mask = (1 << (total_bits - 1));

   /* CC2538 has only 14bit ADC - Last two bits are reserved and always 0. */
   adc_value = adc_get(channel, ref, div) >> 2;

   /* Based on DIV the effective number of bits changes. */
   enb = adc_div_to_enob(div);

   /* Truncate upper bits and sign extend */
   upper_bits = total_bits - enb;
   mask = (1 << upper_bits) - 1;
   is_negative = adc_value & msb_mask;
   if (is_negative) {
      result = adc_value | (mask << enb);
   } else {
      result = adc_value & ((1 << enb) - 1);
   }

   return result;
}

#endif