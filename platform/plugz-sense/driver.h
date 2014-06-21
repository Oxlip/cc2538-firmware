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

double plugz_read_temperature();
uint8_t plugz_read_humidity();
double plugz_read_ambient_lux();

void plugz_sense_driver_init(void);

#endif
