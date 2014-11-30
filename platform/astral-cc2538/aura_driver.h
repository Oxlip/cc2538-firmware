/**
 * \addtogroup Astral
 * @{
 *
 * \file Driver function declarations for various Astral things(Aura, Lyra, Norma etc).
 */

#ifndef AURA_DRIVER_H_
#define AURA_DRIVER_H_

#include "contiki.h"
#include "dev/gpio.h"
#include "dev/nvic.h"
#include <stdio.h>

/**
 * Current sensor reading type.
 */
typedef enum {
  CS_VALUE_TYPE_RMS_CURRENT,    // RMS Current
  CS_VALUE_TYPE_RMS_VOLTAGE,    // RMS Voltage
  CS_VALUE_TYPE_ACTIVE_WATT,    // Active watt
  CS_VALUE_TYPE_VA,             // Instantaneous voltage
  CS_VALUE_TYPE_IA,             // Instantaneous current
  CS_VALUE_TYPE_VA_PEAK,        // Peak voltage
  CS_VALUE_TYPE_IA_PEAK         // Peak current
} CS_VALUE_TYPE;

/* Get current sensor value */
double get_cs_value(CS_VALUE_TYPE type, uint8_t input);

/* Turn on/off triac */
void set_triac(uint8_t triac_no, uint8_t on);

/* Get temperature value */
float get_temperature();

void driver_init(void);

#endif
