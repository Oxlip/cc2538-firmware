/**
 * \addtogroup uSwitch buttons
 * @{
 *
 * \file
 *  Driver for the uSwitch and uPlug buttons.
 *
 *  These buttons are used to turn on/off the given triac.
 */
#include "contiki.h"
#include "dev/nvic.h"
#include "dev/ioc.h"
#include "dev/gpio.h"
#include "dev/button-sensor.h"
#include "sys/timer.h"
#include "lib/sensors.h"
#include "dev/gpio.h"
#include "driver.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BUTTON_SENSOR "Button"

#define button_sensor button_select_sensor
extern const struct sensors_sensor button1_sensor;
extern const struct sensors_sensor button2_sensor;
extern const struct sensors_sensor button3_sensor;
extern const struct sensors_sensor button4_sensor;

#ifdef USING_CC2538DK

#define BUTTON_PORT        GPIO_C_BASE
#define BUTTON_PORT_NO     GPIO_C_NUM
#define BUTTON_VECTOR      NVIC_INT_GPIO_PORT_C
#define BUTTON1_PIN        4
#define BUTTON2_PIN        5
#define BUTTON3_PIN        6
#define BUTTON4_PIN        7

#else

#define BUTTON_PORT        GPIO_B_BASE
#define BUTTON_PORT_NO     GPIO_B_NUM
#define BUTTON_VECTOR      NVIC_INT_GPIO_PORT_B
#define BUTTON1_PIN        2
#define BUTTON2_PIN        3
#define BUTTON3_PIN        4
#define BUTTON4_PIN        5

#endif

static struct timer debouncetimer;
/*---------------------------------------------------------------------------*/
/**
 * \brief Callback registered with the GPIO module. Gets fired with a button
 * port/pin generates an interrupt
 * \param port The port number that generated the interrupt
 * \param pin The pin number that generated the interrupt. This is the pin
 * absolute number (i.e. 0, 1, ..., 7), not a mask
 */
static void
btn_callback(uint8_t port, uint8_t pin)
{
   if(!timer_expired(&debouncetimer)) {
      return;
   }

   timer_set(&debouncetimer, CLOCK_SECOND / 8);
   switch(pin) {
   case BUTTON1_PIN:
      sensors_changed(&button1_sensor);
      break;
   case BUTTON2_PIN:
      sensors_changed(&button2_sensor);
      break;
   case BUTTON3_PIN:
      sensors_changed(&button3_sensor);
      break;
   case BUTTON4_PIN:
      sensors_changed(&button4_sensor);
      break;
   default:
     printf("Unknown switch\n");
   }
}

/*---------------------------------------------------------------------------*/
/**
 * \brief Common initialiser for all buttons
 * \param port_base GPIO port's register offset
 * \param pin_mask Pin mask corresponding to the button's pin
 */
static void
config(uint8_t pin_no)
{
   uint32_t pin_mask = (1 << pin_no);
   uint32_t port_base = BUTTON_PORT;

   /* Software controlled */
   GPIO_SOFTWARE_CONTROL(port_base, pin_mask);

   /* Set pin to input */
   GPIO_SET_INPUT(port_base, pin_mask);

   /* Enable edge detection */
   GPIO_DETECT_EDGE(port_base, pin_mask);

   /* Single edge */
   GPIO_TRIGGER_SINGLE_EDGE(port_base, pin_mask);

   /* Trigger interrupt on Falling edge */
   GPIO_DETECT_RISING(port_base, pin_mask);

   GPIO_ENABLE_INTERRUPT(port_base, pin_mask);

   ioc_set_over(BUTTON_PORT_NO, pin_no, IOC_OVERRIDE_PUE);
   nvic_interrupt_enable(BUTTON_VECTOR);
   gpio_register_callback(btn_callback, BUTTON_PORT_NO, pin_no);
}

/*---------------------------------------------------------------------------*/
/**
 * \brief Init functions for the buttons.
 *
 * Parameters are ignored. They have been included because the prototype is
 * dictated by the core sensor api. The return value is also not required by
 * the API but otherwise ignored.
 */
static int
config_button1(int type, int value)
{
   config(BUTTON1_PIN);
   return 1;
}

static int
config_button2(int type, int value)
{
   config(BUTTON2_PIN);
   return 1;
}

static int
config_button3(int type, int value)
{
   config(BUTTON3_PIN);
   return 1;
}

static int
config_button4(int type, int value)
{
   config(BUTTON4_PIN);
   return 1;
}
/*---------------------------------------------------------------------------*/
void
button_init()
{
  timer_set(&debouncetimer, 0);
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button1_sensor, BUTTON_SENSOR, NULL, config_button1, NULL);
SENSORS_SENSOR(button2_sensor, BUTTON_SENSOR, NULL, config_button2, NULL);
SENSORS_SENSOR(button3_sensor, BUTTON_SENSOR, NULL, config_button3, NULL);
SENSORS_SENSOR(button4_sensor, BUTTON_SENSOR, NULL, config_button4, NULL);

SENSORS(&button1_sensor, &button2_sensor, &button3_sensor, &button4_sensor);
