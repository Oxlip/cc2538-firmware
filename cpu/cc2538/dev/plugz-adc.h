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
   const uint16_t resolution = (1 << (enob-1)) - 1;
   const double volts_per_bit = ref_mv / (resolution << (16 - enob));

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

#endif
