/**
 * @{
 *
 * \defgroup Astral
 *
 * 
 * @{
 *
 * \file Driver for Dimming an light/fan using Triac and Zero crossing interrupt.
  */
#include "dimmer.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int dimmer_configured = 0;

static uint32_t dimmer_cb_granularity_ms;
static uint32_t rt_time_ms;

#define TRIAC_ON     1
#define TRIAC_OFF    0

/*
 * \brief Zero Cross timer callback.
 *
 * This callback is invoked when it is time to turn on Triac.
 *
 * \note This wont be called when dimming option is either 0 or 100%.
 *
 * \param rt    Timer.
 * \param ptr   unused.
 */
void dimmer_timer_callback( struct rtimer *rt, void* ptr)
{
   int device =(dimmer_config_t *)rt - &dimmer_config[0];
   set_triac(device, TRIAC_ON);
}

/*
 * \brief Zero Cross ISR callback.
 *
 * When the Zero Cross circuit detects AC sine wave's zero cross it generates an
 * interrupt which is handled by an ISR which calls this function.
 * So this function will be called at 50Hz frequency(100 times per second).
 * This function schedules a RT timer, such that it will be fired when
 * the TRIAC has to be turned on.
 *
 * \param port  The port number that generated the interrupt.
 * \param pin   The pin number that generated the interrupt. This is the pin
 *              absolute number (i.e. 0, 1, ..., 7), not a mask.
 */
static void
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
         set_triac(i, TRIAC_OFF);
         rtimer_expire = (dimmer_cb_granularity_ms * dimmer_config[i].percent) / rt_time_ms;

         result = rtimer_set(&dimmer_config[i].rt, RTIMER_NOW() + rtimer_expire + 2, 1,
                             (rtimer_callback_t)dimmer_timer_callback, NULL);
         if(result != RTIMER_OK) {
            PRINTF("Error Setting Rtimer for device %d\n", i);
         }
      }
   }
}

/*
 * \brief Enable dimmer and set the brightness.
 *
 * \param triac     Triac number(0-4).
 * \param percent   Brightness(0-100).
 */
void
dimmer_enable(int triac, int percent)
{
   /* Already enabled, and no change in percent,  nothing to do, return.*/
   if(dimmer_config[triac].enabled == 1 && dimmer_config[triac].percent == percent) {
      return;
   }

   if (!dimmer_config[triac].enabled) {
      dimmer_configured++;
      dimmer_config[triac].enabled = 1;
   }

   dimmer_config[triac].percent = percent;

   if(dimmer_config[triac].percent == 100) {
      set_triac(triac, TRIAC_OFF);
   }

   /* If this is the first triac that needs to be dimmed, enable the zero cross interrupt */
   if (dimmer_configured == 1) {
      nvic_interrupt_enable(ZERO_CROSS_VECTOR);
   }
}

/*
 * \brief Disable dimmer.
 *
 * \param triac     Triac number(0-4).
 */
void
dimmer_disable(int triac)
{
   if (dimmer_config[triac].enabled == 0) {
      return;
   }
   dimmer_config[triac].enabled = 0;
   dimmer_config[triac].percent = 0;

   set_triac(triac, TRIAC_ON);

   dimmer_configured--;
   /* If this is the last triac that is being disabled, disable the zero cross interrupt */
   if (dimmer_configured == 0) {
      nvic_interrupt_disable(ZERO_CROSS_VECTOR);
   }
}

/*
 * \brief Initialize the dimmer code.
 */
void
dimmer_init(uint8_t ac_frequency)
{
   /* Can be a macro if zc_frequency is known a-priori, else need
    * to initialize these based on calibration. TODO
    */
   uint8_t  zc_frequency;
   uint32_t zc_interval_ms;

   /* Configure Zero Cross pin as input */
   GPIO_SOFTWARE_CONTROL(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_SET_INPUT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

   /* Trigger interrupt on falling edge */
   GPIO_DETECT_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_TRIGGER_SINGLE_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_DETECT_RISING(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
   GPIO_ENABLE_INTERRUPT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

   //ioc_set_over(ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PINZERO_CROSS_GPIO_PIN, IOC_OVERRIDE_PUE);
   gpio_register_callback(zero_cross_handler, ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PIN);

   /* The number of times Zero crossing interrupt will be called per second. */
   zc_frequency = 2 * ac_frequency;

   /* Time in microseconds between Zero cross Interrupts, here it's 10 pow 4 */
   zc_interval_ms = 1000000UL / zc_frequency;

   /* Time in microseconds when we divide the interval into 100 equal parts
    * Here it would be 100 micro seconds */
   dimmer_cb_granularity_ms = zc_interval_ms / 100;

   /* Time in microseconds between each RT tick, here it is 30 usec */
   rt_time_ms = 1000000UL / RTIMER_ARCH_SECOND;
}

