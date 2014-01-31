/**
 * @{
 *
 * \defgroup plugz-switch Dimmer Driver
 *
 * Driver for Dimmer Application
 * @{
 *
 * \file
 * plugz-switch Dimmer Driver
 */
#include "dimmer.h"
#include "driver.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int dimmer_configured = 0;

/*
 * \brief Callback registered with the Zero Cross detection.
 * \param port The port number that generated the interrupt
 * \param pin The pin number that generated the interrupt. This is the pin
 * absolute number (i.e. 0, 1, ..., 7), not a mask
 */
static void
zero_cross_detected(uint8_t port, uint8_t pin)
{
   zero_cross_handler(port, pin);
}

void
dimmer_init()
{

   /* Configure Zero Cross pin as input */
   GPIO_SOFTWARE_CONTROL(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_SET_INPUT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

   /* Trigger interrupt on falling edge */
   GPIO_DETECT_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_TRIGGER_SINGLE_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_DETECT_RISING(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_ENABLE_INTERRUPT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

   //ioc_set_over(ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PINZERO_CROSS_GPIO_PIN, IOC_OVERRIDE_PUE);
   gpio_register_callback(zero_cross_detected, ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PIN);

   /* Enable based on input */
   //nvic_interrupt_enable(ZERO_CROSS_VECTOR);

   /* TODO: Do we need to dynamically calculate the frequency? */

   zero_crossing_frequency = 2 * REGIONAL_VOLTAGE_FREQUENCY;

   dimmer_time_ms = 1000 / zero_crossing_frequency;
}

void
dimmer_callback(/* ctimer or rtimer? */ void* ptr)
{
   int i;
   for(i = 0; i < MAX_TRIACS; i++) {
         plugz_triac_turn_on(i+1);
   }
}

/*
 * When the Zero Cross circuit detects AC sin wave's zero cross it generates an
 * interrupt which is handled by an ISR(look at platform/plugz-switch/) which
 * calls this function. So this function will be called at 50Hz
 * frequency(100 times * per second).
 */
void
zero_cross_handler(uint8_t port, uint8_t pin)
{
   int i;
#ifdef DEBUG
   static int intr_count = 0;
   intr_count++;
   /* Print every 5 seconds */
   if (intr_count % 500) {
      PRINTF("zero_cross_handler invoked %d times, port = %d, pin = %d\n",
             intr_count, port,pin);
   }
#endif /* DEBUG */
   for (i = 0; i< MAX_TRIACS; i++) {
      if (dimmer_config[i].enabled == 1) {
            plugz_triac_turn_off(i + 1);
            /* TODO : Set Timer to expire at 1/4 the Cycle Time
             */
      }
   }

}

void
dimmer_enable(int triac, int step)
{
   /* Already enabled, so nothing to do, return.
    *  TODO Allow change in step value
    */
   if(dimmer_config[triac].enabled == 1) {
      return;
   }

   dimmer_configured++;

   /* If this is the first triac that needs to be dimmed, enable the zero
    * cross interrupt
    */
   if(dimmer_configured == 1) {
       nvic_interrupt_enable(ZERO_CROSS_VECTOR);
   }
}

void
dimmer_disable(int triac)
{
   if (dimmer_config[triac].enabled == 0)
      return;

   dimmer_config[triac].enabled = 0;
   dimmer_config[triac].step = 0;

   dimmer_configured--;

   /* If this is the last triac that is being disabled,
      disable the zero cross interrupt
    */
   if(dimmer_configured == 0) {
      nvic_interrupt_disable(ZERO_CROSS_VECTOR);
   }
}
