/**
 * \addtogroup Astral
 * @{
 *
 * \defgroup Astral Buttons
 *
 */
#ifndef _ASTRAL_BUTTON_SENSOR_H_
#define _ASTRAL_BUTTON_SENSOR_H_

#include "lib/sensors.h"
#include "dev/gpio.h"

extern const struct sensors_sensor button1_sensor;
extern const struct sensors_sensor button2_sensor;
extern const struct sensors_sensor button3_sensor;
extern const struct sensors_sensor button4_sensor;

/** \brief Common initialiser for all buttons */
void button_init();

#endif /* BUTTON_SENSOR_H_ */

/**
 * @}
 * @}
 */