/**
 * \addtogroup Astral
 * @{
 *
 * \file Mira driver function declarations.
 */

#ifndef USENSE_DRIVER_H_
#define USENSE_DRIVER_H_

#include "contiki.h"
#include "dev/gpio.h"
#include "dev/nvic.h"
#include <stdio.h>

/* Reads si7013 and returns temperature and humidity */
int read_si7013(float *temperature, int32_t *humidity);

/* Returns ambient light value in lux */
float get_ambient_lux();
/* Converts lux to percentage */
float lux_to_pct(float);

void driver_init(void);

#endif
