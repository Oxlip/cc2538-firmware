/**
 * @{
 *
 * \defgroup Astral
 *
 * @{
 *
 * \file Declaration file for the Aura and Norma lighter dimmer driver.
 */
#ifndef ASTRAL_DIMMER_H_
#define ASTRAL_DIMMER_H_
#include "aura_driver.h"

#define MAX_TRIACS                  4

#define ZERO_CROSS_GPIO_BASE        GPIO_C_BASE
#define ZERO_CROSS_GPIO_PIN         7
#define ZERO_CROSS_GPIO_PIN_MASK    (1 << ZERO_CROSS_GPIO_PIN)
#define ZERO_CROSS_PORT_NUM         GPIO_C_NUM
#define ZERO_CROSS_VECTOR           NVIC_INT_GPIO_PORT_C

typedef struct {
   struct   rtimer rt;
   uint8_t  enabled;
   int      percent;
} dimmer_config_t;
dimmer_config_t dimmer_config[MAX_TRIACS];

extern void dimmer_init(uint8_t ac_frequency);
extern void dimmer_enable(int triac, int percent);
extern void dimmer_disable(int triac);

#endif /* DIMMER_H_ */
