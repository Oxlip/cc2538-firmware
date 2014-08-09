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
 * \addtogroup cc2538
 * @{
 *
 * \defgroup cc2538-adc cc2538 ADC
 *
 * Driver for the cc2538 ADC controller
 * @{
 *
 * \file
 * Header file for the cc2538 ADC driver
 */
#ifndef ADC_H_
#define ADC_H_

#include "contiki.h"
#include "dev/soc-adc.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/** \name ADC functions
 * @{
 */

/** \brief Initializes the ADC controller */
void adc_init(void);

/** \brief Performs a single conversion on a given ADC channel
 * \param channel The channel used for the conversion: \c SOC_ADC_ADCCON_CH_x
 * \param ref The reference voltage used for the conversion: \c SOC_ADC_ADCCON_REF_x
 * \param div The decimation rate used for the conversion: \c SOC_ADC_ADCCON_DIV_x
 * \return Signed 16-bit conversion result: 2's complement, ENOBs in MSBs
 * \note PD[5:4] are not usable when the temperature sensor is selected.
 */
int16_t adc_get(uint8_t channel, uint8_t ref, uint8_t div);

/*
 * Returns Vdd supplied to CC2538.
 *
 * By using cc2538's internal voltage reference(1.19v) as reference voltage and
 * internal channel VDD/3, we can get the Vcc.
 *
 * Result is returned in milivolt units.
 */
double get_vdd();

/*
 * Converts given DIV to ENOB.
 */
uint8_t adc_div_to_enob(uint8_t div);

/*
 * Converts given adc_value to milivolt sensed at the ADC pin.
 *  \param ref_voltate  Reference voltage used(in mv eg: 3300mv).
 *  \param enob         Effective number of bits.
 */
static inline double
adc_to_volt(int16_t adc_value, double ref_mv, int enob)
{
   const uint16_t resolution = (1 << (enob-1)) - 1;
   const double volts_per_bit = ref_mv / (resolution << (16 - enob));

   return (double)adc_value * volts_per_bit;
}

/** @} */

#endif /* ADC_H_ */

/**
 * @}
 * @}
 */
