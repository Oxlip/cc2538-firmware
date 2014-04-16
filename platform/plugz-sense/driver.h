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

float plugz_read_temperature_sensor_value();

void plugz_sense_driver_init(void);

#endif
