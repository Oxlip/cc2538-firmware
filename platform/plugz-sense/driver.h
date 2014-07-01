/**
 * \addtogroup plugz-usense
 * @{
 *
 * \file
 *  PlugZ uSense Driver function declarations.
 */

#ifndef PLUGZ_DRIVER_H_
#define PLUGZ_DRIVER_H_

#include "contiki.h"
#include "dev/gpio.h"
#include "dev/nvic.h"
#include <stdio.h>


int plugz_read_si7013(float *temperature, int32_t *humdity);
float plugz_read_ambient_lux();
float plugz_lux_to_pct(float);

void plugz_sense_driver_init(void);

#endif
