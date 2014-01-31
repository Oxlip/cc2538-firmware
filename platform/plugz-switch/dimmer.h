/**
 * @{
 *
 * \defgroup plugz-switch Dimmer Driver
 *
 * Driver for Dimmer Application
 * @{
 *
 * \file
 * Header file for the plugz-switch Dimmer Driver
 */
#ifndef DIMMER_H_
#define DIMMER_H_
#include "driver.h"


#define MAX_TRIACS            4

#define ZERO_CROSS_GPIO_BASE        GPIO_C_BASE
#define ZERO_CROSS_GPIO_PIN         7
#define ZERO_CROSS_GPIO_PIN_MASK    (1 << ZERO_CROSS_GPIO_PIN)
#define ZERO_CROSS_PORT_NUM         GPIO_C_NUM
#define ZERO_CROSS_VECTOR           NVIC_INT_GPIO_PORT_C

extern void dimmer_init();
extern void zero_cross_handler(uint8_t port, uint8_t pin);
extern void dimmer_enable(int triac, int percent);
extern void dimmer_disable(int triac);

typedef struct {
   uint8_t  enabled;
   int      percent;
} dimmer_config_t;
dimmer_config_t dimmer_config[MAX_TRIACS];

uint8_t  zero_crossing_frequency;
uint8_t dimmer_time_ms;

#define REGIONAL_VOLTAGE_FREQUENCY 50

#endif /* DIMMER_H_ */
