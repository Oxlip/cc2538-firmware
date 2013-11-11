/**
 * \addtogroup plugz-switch
 * @{
 *
 * \defgroup plugz-switch driver
 *
 * PlugZ Switch driver
 * @{
 *
 * \file
 * Triac, Current sensor, Temp sensor and Switch driver for PlugZ-Switch board
 */
#include "contiki.h"
#include "reg.h"
#include "dev/gpio.h"

#define TRIAC_GPIO_BASE 			GPIO_C_BASE
#define TRIAC1_GPIO_PIN				0
#define TRIAC2_GPIO_PIN				1
#define TRIAC3_GPIO_PIN				2
#define TRIAC4_GPIO_PIN				3
#define TRIAC_GPIO_PIN_MASK			0b1111

#define ZERO_CROSS_GPIO_BASE		GPIO_C_BASE
#define ZERO_CROSS_GPIO_PIN 		7
#define ZERO_CROSS_GPIO_PIN_MASK 	(1 << ZERO_CROSS_GPIO_PIN)
#define ZERO_CROSS_PORT_NUM			GPIO_C_NUM
#define ZERO_CROSS_VECTOR 			NVIC_INT_GPIO_PORT_C

#define CURRENT_SENSOR_GPIO_BASE	GPIO_A_BASE
#define CURRENT_SENSOR1_GPIO_PIN	2
#define CURRENT_SENSOR2_GPIO_PIN	3
#define CURRENT_SENSOR3_GPIO_PIN	4
#define CURRENT_SENSOR4_GPIO_PIN	5
#define CURRENT_SENSOR_GPIO_PIN_MASK   0b111100

/**
 * \brief Callback registered with the Zero Cross detection. 
 * \param port The port number that generated the interrupt
 * \param pin The pin number that generated the interrupt. This is the pin
 * absolute number (i.e. 0, 1, ..., 7), not a mask
 */
static void
zero_cross_detected(uint8_t port, uint8_t pin)
{

}

/*---------------------------------------------------------------------------*/
void
plugz_switch_driver_init(void)
{
	/* Configure TRIAC pins as output */
	GPIO_SOFTWARE_CONTROL(TRIAC_GPIO_BASE, TRIAC_GPIO_PIN_MASK);
	GPIO_SET_OUTPUT(TRIAC_GPIO_BASE, TRIAC_GPIO_PIN_MASK);

	/* Configure Zero Cross pin as input and trigger interrupt on falling edge */
	GPIO_SOFTWARE_CONTROL(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
	GPIO_SET_INPUT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
#if 0
	GPIO_DETECT_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
	GPIO_TRIGGER_SINGLE_EDGE(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
	GPIO_DETECT_RISING(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);
	GPIO_ENABLE_INTERRUPT(ZERO_CROSS_GPIO_BASE, ZERO_CROSS_GPIO_PIN_MASK);

	ioc_set_over(ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PINZERO_CROSS_GPIO_PIN, IOC_OVERRIDE_PUE);
	nvic_interrupt_enable(ZERO_CROSS_VECTOR);
	gpio_register_callback(zero_cross_detected, ZERO_CROSS_PORT_NUM, ZERO_CROSS_GPIO_PIN);
#endif

	/* Configure current sensors as input */
	GPIO_SOFTWARE_CONTROL(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);
	GPIO_SET_INPUT(CURRENT_SENSOR_GPIO_BASE, CURRENT_SENSOR_GPIO_PIN_MASK);

	button_init();
}
/*---------------------------------------------------------------------------*/
void
plugz_triac_turn_on(uint8_t triac_no)
{
	REG((TRIAC_GPIO_BASE | GPIO_DATA) + (1 << triac_no)) = 0xFF;
}
/*---------------------------------------------------------------------------*/
void
plugz_triac_turn_off(uint8_t triac_no)
{
	REG((TRIAC_GPIO_BASE | GPIO_DATA) + (1 << triac_no)) = 0;
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */
