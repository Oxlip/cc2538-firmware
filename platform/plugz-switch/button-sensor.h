/**
 * \addtogroup uSwitch
 * @{
 *
 * \defgroup uSwitch Button Driver
 *
 * Driver for the uSwitch/uPlug buttons
 * @{
 *
 * \file
 * Header file for the uSwitch/uPlug Button Driver
 */
#ifndef BUTTON_SENSOR_H_
#define BUTTON_SENSOR_H_

#include "lib/sensors.h"
#include "dev/gpio.h"

#define BUTTON_SENSOR "Button"

extern const struct sensors_sensor button1_sensor;
extern const struct sensors_sensor button2_sensor;
extern const struct sensors_sensor button3_sensor;
extern const struct sensors_sensor button4_sensor;
/*---------------------------------------------------------------------------*/
#endif /* BUTTON_SENSOR_H_ */

/** \brief Common initialiser for all buttons */
void button_init();

/**
 * @}
 * @}
 */