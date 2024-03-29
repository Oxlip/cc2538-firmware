/*
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
 * Benoît Thébaudeau <benoit.thebaudeau@advansee.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup cc2538-adc
 * @{
 *
 * \file
 * Implementation of the cc2538 ADC driver
 */
#include "contiki.h"
#include "dev/soc-adc.h"
#include "dev/cctest.h"
#include "dev/rfcore-xreg.h"
#include "dev/adc.h"
#include "reg.h"

#include <stdint.h>

/*---------------------------------------------------------------------------*/
void
adc_init(void)
{
  /* Start conversions only manually */
  REG(SOC_ADC_ADCCON1) |= SOC_ADC_ADCCON1_STSEL;
}
/*---------------------------------------------------------------------------*/
int16_t
adc_get(uint8_t channel, uint8_t ref, uint8_t div)
{
  uint32_t cctest_tr0, rfcore_xreg_atest, reg;
  uint16_t high, low;

  /* On-chip temperature sensor */
  if(channel == SOC_ADC_ADCCON_CH_TEMP) {
    /* Connect the temperature sensor to the ADC */
    cctest_tr0 = REG(CCTEST_TR0);
    REG(CCTEST_TR0) = cctest_tr0 | CCTEST_TR0_ADCTM;

    /* Enable the temperature sensor */
    rfcore_xreg_atest = REG(RFCORE_XREG_ATEST);
    REG(RFCORE_XREG_ATEST) = (rfcore_xreg_atest & ~RFCORE_XREG_ATEST_ATEST_CTRL) |
                             RFCORE_XREG_ATEST_ATEST_CTRL_TEMP;
  }

  /* Start a single extra conversion with the given parameters */
  reg = REG(SOC_ADC_ADCCON3) & ~(SOC_ADC_ADCCON3_EREF | SOC_ADC_ADCCON3_EDIV | SOC_ADC_ADCCON3_ECH);
  REG(SOC_ADC_ADCCON3) = reg | ref | div | channel;

  /* Poll until end of conversion */
  while(!(REG(SOC_ADC_ADCCON1) & SOC_ADC_ADCCON1_EOC));

  /* Read conversion result, reading SOC_ADC_ADCH last to clear
   * SOC_ADC_ADCCON1.EOC */
  low = REG(SOC_ADC_ADCL) & 0xfc;
  high = REG(SOC_ADC_ADCH) & 0xff;

  /* On-chip temperature sensor */
  if(channel == SOC_ADC_ADCCON_CH_TEMP) {
    /* Restore the initial temperature sensor state and connection (better for
     * power consumption) */
    REG(RFCORE_XREG_ATEST) = rfcore_xreg_atest;
    REG(CCTEST_TR0) = cctest_tr0;
  }

  /* Return conversion result */
  return (high << 8) | low ;
}

uint8_t
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
 * Returns internal voltage of the CC2538.
 */
static double
cc2538_internal_voltage()
{
   /* Read cc2538 datasheet for internal ref variation:
    * (1.19v) + vdd coeffient(2mv per v). + temp coefficent
    */
   /* TODO - Include temp coefficent and vdd coefficent*/
   return 1190;
}

double
get_vdd()
{
   int16_t adc_value;
   double pa_mv;

   adc_value = adc_get(SOC_ADC_ADCCON_CH_VDD_3, SOC_ADC_ADCCON_REF_INT, SOC_ADC_ADCCON_DIV_512);
   pa_mv = adc_to_volt(adc_value, cc2538_internal_voltage(), adc_div_to_enob(SOC_ADC_ADCCON_DIV_512));
   return pa_mv * 3;
}

/** @} */
