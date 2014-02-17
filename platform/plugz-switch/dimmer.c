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

   /* TODO: Do we need to dynamically calculate the frequency? */

   /* The number of times Zero crossing interrupt will be called per second
    *, for 50Hz, it's 100 times */
   zero_crossing_frequency = 2 * REGIONAL_VOLTAGE_FREQUENCY;

   /* Time in microseconds between Zero cross Interrupts, here it's 10 pow
    * 4 */
   zero_crossing_interval_micro_second = 1000000UL / zero_crossing_frequency;

   /* Time in microseconds when we divide the interval into 100 equal parts
    * Here it would be 100 micro seconds */
   dimmer_callback_granularity_micro_second =
      zero_crossing_interval_micro_second/100;

   /* Time in microseconds between each RT tick, here it is 30 usec */
   rt_clock_time_in_microseconds = 1000000UL / RTIMER_ARCH_SECOND;
}

void dimmer_callback( struct rtimer *rt, void* ptr)
{
   int device =(dimmer_config_t *)rt - &dimmer_config[0];
   plugz_triac_turn_on(device);
}

/*
 * When the Zero Cross circuit detects AC sine wave's zero cross it generates an
 * interrupt which is handled by an ISR which calls this function.
 * So this function will be called at 50Hz frequency(100 times per second).
 */
void
zero_cross_handler(uint8_t port, uint8_t pin)
{
   int i;
   rtimer_clock_t rtimer_expire = 0;
   int result;

   for(i = 0; i < MAX_TRIACS; i++)
   {
      /* For Dim percentage of 100 we don't start the timer,
       *  for the rest the timer fires at the closest approximation.
       */
      if(dimmer_config[i].enabled == 1 && dimmer_config[i].percent != 100) {
         plugz_triac_turn_off(i);
         rtimer_expire = (dimmer_callback_granularity_micro_second * dimmer_config[i].percent) / rt_clock_time_in_microseconds;

         result = rtimer_set(&dimmer_config[i].rt, RTIMER_NOW() + rtimer_expire + 2, 1,
                             (rtimer_callback_t)dimmer_callback, NULL);
         if(result != RTIMER_OK) {
            PRINTF("Error Setting Rtimer for device %d\n", i);
         }
      }
   }

}

void dimmer_enable(int triac, int percent)
{
   /* Already enabled, and no change in percent,  nothing to do, return.
    */
   if(dimmer_config[triac].enabled == 1 &&
      dimmer_config[triac].percent == percent) {
      return;
   }

   if (!dimmer_config[triac].enabled) {
      dimmer_configured++;
      dimmer_config[triac].enabled = 1;
   }

   dimmer_config[triac].percent = percent;

   if(dimmer_config[triac].percent == 100) {
      plugz_triac_turn_off(triac);
   }

   /* If this is the first triac that needs to be dimmed, enable the zero
    * cross interrupt
    */
   if (dimmer_configured == 1) {
      nvic_interrupt_enable(ZERO_CROSS_VECTOR);
   }
}

void
dimmer_disable(int triac)
{
   PRINTF(" dimmer_disable: %d\n", triac);
   if (dimmer_config[triac].enabled == 0) {
      return;
   }

   dimmer_config[triac].enabled = 0;

   dimmer_config[triac].percent = 0;

   plugz_triac_turn_on(triac);

   dimmer_configured--;

   /* If this is the last triac that is being disabled,
      disable the zero cross interrupt
    */
   if (dimmer_configured == 0) {
      nvic_interrupt_disable(ZERO_CROSS_VECTOR);
   }
}
