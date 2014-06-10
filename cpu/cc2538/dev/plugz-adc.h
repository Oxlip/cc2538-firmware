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
 *  ref_voltate - Reference voltage used(in mv eg: 3300mv).
 *  enob - Effective number of bits.
 */
static inline double
adc_to_volt(int16_t adc_value, double ref_mv, int enob)
{
   const double resolution = (1 << (enob-1)) - 1;
   const double volts_per_bit = ref_mv / resolution;

   return (double)adc_value * volts_per_bit;
}

/*
 * Converts given DIV to ENOB.
 */
static uint8_t
adc_div_to_enob(uint8_t div)
{
   switch (div) {
      case SOC_ADC_ADCCON_DIV_512:
         return 12;
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
   int16_t adc_value;
   uint8_t enb, rshift;

   /* Based on DIV the effective number of bits changes. */
   enb = adc_div_to_enob(div);
   rshift = 16 - enb;
   /* CC2538 has only 14bit ADC - Last two bits are reserved and always 0. */
   adc_value = adc_get(channel, ref, div);
   return adc_value >> rshift;
}

#endif
