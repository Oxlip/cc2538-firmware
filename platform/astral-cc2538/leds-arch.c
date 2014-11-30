/**
 * \addtogroup Astral
 * @{
 *
 * \defgroup Astral LED driver
 *
 * @{
 *
 * \file LED driver implementation for the Astral things.
 */
#include "dev/leds.h"
#include "dev/gpio.h"

#define LEDS_GPIO_BASE		 GPIO_C_BASE
#define LEDS_GPIO_PIN_MASK   LEDS_ALL

void
leds_arch_init(void)
{
  GPIO_SET_OUTPUT(LEDS_GPIO_BASE, LEDS_GPIO_PIN_MASK);
}

unsigned char
leds_arch_get(void)
{
  return GPIO_READ_PIN(LEDS_GPIO_BASE, LEDS_GPIO_PIN_MASK);
}

void
leds_arch_set(unsigned char leds)
{
  GPIO_WRITE_PIN(LEDS_GPIO_BASE, LEDS_GPIO_PIN_MASK, leds);
}

/**
 * @}
 * @}
 */
