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

#define MAX_TRIACS            4

#define ZERO_CROSS_GPIO_BASE        GPIO_C_BASE
#define ZERO_CROSS_GPIO_PIN         7
#define ZERO_CROSS_GPIO_PIN_MASK    (1 << ZERO_CROSS_GPIO_PIN)
#define ZERO_CROSS_PORT_NUM         GPIO_C_NUM
#define ZERO_CROSS_VECTOR           NVIC_INT_GPIO_PORT_C

extern void zero_cross_handler(uint8_t port, uint8_t pin);
extern void dimmer_enable(int triac, int step);
extern void dimmer_disable(int triac);

static int    dimmer_configured = 0;

typedef struct {
   uint8_t enabled;
   uint8_t  step;
} dimmer_config_t;
dimmer_config_t dimmer_config[MAX_TRIACS];

#endif /* DIMMER_H_ */
