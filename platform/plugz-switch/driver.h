/**
 * \addtogroup uSwitch
 * @{
 *
 * \file
 *  uPlug's and uSwitch's Driver function declarations.
 */

#ifndef USWITCH_DRIVER_H_
#define USWITCH_DRIVER_H_

#include "contiki.h"
#include "dev/gpio.h"
#include "dev/nvic.h"
#include <stdio.h>

void set_triac(uint8_t triac_no, uint8_t on);

double get_current_sensor_value();
float get_temperature();

double get_vdd();

void driver_init(void);

#endif
