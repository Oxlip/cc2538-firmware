/**
 * \addtogroup uSense
 * @{
 *
 * \defgroup uSense LED driver
 *
 * LED driver implementation for the uSense
 * @{
 *
 * \file
 * LED driver implementation for the uSense
 */
#include "dev/leds.h"
#include "dev/gpio.h"

#define LEDS_GPIO_PIN_MASK   LEDS_ALL

void
leds_arch_init(void)
{
  GPIO_SET_OUTPUT(GPIO_C_BASE, LEDS_GPIO_PIN_MASK);
}

unsigned char
leds_arch_get(void)
{
  return GPIO_READ_PIN(GPIO_C_BASE, LEDS_GPIO_PIN_MASK);
}

void
leds_arch_set(unsigned char leds)
{
  GPIO_WRITE_PIN(GPIO_C_BASE, LEDS_GPIO_PIN_MASK, leds);
}

/**
 * @}
 * @}
 */
