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

#include "contiki.h"
#include "reg.h"
#include "dev/leds.h"
#include "dev/gpio.h"
#include "dev/ioc.h"

#define LED_POWER_PIN               5
#define LED_LINK_UP_PIN             6
#define LED_STATUS_PIN              7

#define LEDS_GPIO_BASE              GPIO_C_BASE
#define LEDS_PORT                   GPIO_C_NUM
#define LEDS_GPIO_PIN_MASK_IN       ((1 << LED_LINK_UP_PIN) | (1 << LED_STATUS_PIN))
#define LEDS_GPIO_PIN_MASK_OUT      (1 << LED_POWER_PIN)

static int led_status = 0;
void
leds_arch_init(void)
{
  GPIO_SOFTWARE_CONTROL(LEDS_GPIO_BASE, LEDS_GPIO_PIN_MASK_IN | LEDS_GPIO_PIN_MASK_OUT);

  GPIO_SET_INPUT(LEDS_GPIO_BASE, LEDS_GPIO_PIN_MASK_IN);
  ioc_set_over(LEDS_PORT, LED_LINK_UP_PIN, IOC_OVERRIDE_PUE);
  ioc_set_over(LEDS_PORT, LED_STATUS_PIN, IOC_OVERRIDE_PUE);

  GPIO_SET_OUTPUT(LEDS_GPIO_BASE, LEDS_GPIO_PIN_MASK_OUT);
}

unsigned char
leds_arch_get(void)
{
  return led_status;
}

void
leds_arch_set(unsigned char leds)
{
  int power, linkup, status;

  power = (leds & LEDS_RED) ? LED_POWER_PIN : 0;
  GPIO_WRITE_PIN(GPIO_C_BASE, LEDS_GPIO_PIN_MASK_OUT, power);

  linkup = (leds & LEDS_YELLOW) ? IOC_OVERRIDE_PDE : IOC_OVERRIDE_PUE;
  ioc_set_over(LEDS_PORT, LED_STATUS_PIN, linkup);

  status = (leds & LEDS_GREEN) ? IOC_OVERRIDE_PDE : IOC_OVERRIDE_PUE;
  ioc_set_over(LEDS_PORT, LED_LINK_UP_PIN, status);

  led_status = leds;
}

/**
 * @}
 * @}
 */
