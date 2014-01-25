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

static int    dimmer_configured = 0;

typedef struct {
   uint8_t enabled;
   uint8_t  step;
} dimmer_config_t;
dimmer_config_t dimmer_config[MAX_TRIACS];

/*
 * When the Zero Cross circuit detects AC sin wave's zero cross it generates an
 * interrupt which is handled by an ISR(look at platform/plugz-switch/) which
 * calls this function. So this function will be called at 50Hz
 * frequency(100 times * per second).
 */
void
zero_cross_handler(uint8_t port, uint8_t pin)
{

}

void dimmer_enable(int triac, int step)
{
   /* Already enabled, so nothing to do, return.
    *  TODO Allow change in step value
    */
   if(dimmer_config[triac].enabled == 1)
      return;

   dimmer_configured++;

   /* If this is the first triac that needs to be dimmed, enable the zero
    * cross interrupt
    */
   if(dimmer_configured == 1)
       nvic_interrupt_enable(ZERO_CROSS_VECTOR);
}

void dimmer_disable(int triac)
{
   if (0 == dimmer_config[triac].enabled)
      return;

   dimmer_config[triac].enabled = 0;
   dimmer_config[triac].step = 0;

   dimmer_configured--;

   /* If this is the last triac that is being disabled, 
      disable the zero cross interrupt
    */
   if(0 == dimmer_configured)
      nvic_interrupt_disable(ZERO_CROSS_VECTOR);
}
