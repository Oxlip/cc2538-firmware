/**
 * \addtogroup plugz
 * @{
 *
 * \file
 *  PlugZ Switch Driver function declarations.
 */

#ifndef PLUGZ_DRIVER_H_
#define PLUGZ_DRIVER_H_

#include "contiki.h"
#include "dev/gpio.h"
#include "dev/nvic.h"
#include <stdio.h>

void plugz_triac_turn_on(uint8_t triac_no);
void plugz_triac_turn_off(uint8_t triac_no);

double plugz_read_current_sensor_value();
float plugz_read_temperature_sensor_value();

double plugz_read_internal_voltage();

void driver_init(void);

#endif
