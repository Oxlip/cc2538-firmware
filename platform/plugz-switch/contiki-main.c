/**
 * \addtogroup platform
 * @{
 *
 * \defgroup PlugZ-Switch
 *
 * PlugZ-Switch is a 4 triac device based on cc2538.
 * @{
 *
 * \file
 *   Main module for the PlugZ-Switch
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/leds.h"
#include "dev/sys-ctrl.h"
#include "dev/scb.h"
#include "dev/nvic.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/ioc.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/cc2538-rf.h"
#include "dev/udma.h"
#include "usb/usb-serial.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/tcpip.h"
#include "net/uip.h"
#include "net/mac/frame802154.h"
#include "cpu.h"
#include "reg.h"
#include "ieee-addr.h"
#include "lpm.h"
#include "lib/sensors.h"
#include "uip-ds6.h"
#include "driver.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/

#if UART_CONF_ENABLE
#define PUTS(s) puts(s)
#else
#define PUTS(s)
#endif
/*---------------------------------------------------------------------------*/
static void
set_rime_addr()
{
  ieee_addr_cpy_to(&rimeaddr_node_addr.u8[0], RIMEADDR_SIZE);

#if STARTUP_CONF_VERBOSE
  {
    int i;
    printf("Rime configured with address ");
    for(i = 0; i < RIMEADDR_SIZE - 1; i++) {
      printf("%02x:", rimeaddr_node_addr.u8[i]);
    }
    printf("%02x\n", rimeaddr_node_addr.u8[i]);
  }
#endif

}

static void
print_net_info()
{
#if STARTUP_CONF_VERBOSE
  printf(" Net: %s\n", NETSTACK_NETWORK.name);
  printf(" MAC: %s\n", NETSTACK_MAC.name);
  printf(" RDC: %s\n", NETSTACK_RDC.name);

  printf("Tentative link-local IPv6 address ");
  {
    uip_ds6_addr_t *lladdr;
    int i;
    lladdr = uip_ds6_get_link_local(-1);
    for(i = 0; i < 7; ++i) {
      printf("%02x%02x:", lladdr->ipaddr.u8[i * 2],
             lladdr->ipaddr.u8[i * 2 + 1]);
    }
    printf("%02x%02x\n", lladdr->ipaddr.u8[14], lladdr->ipaddr.u8[15]);
  }

  if(!UIP_CONF_IPV6_RPL) {
    uip_ipaddr_t ipaddr;
    int i;
    uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
    uip_ds6_addr_add(&ipaddr, 0, ADDR_TENTATIVE);
    printf("Tentative global IPv6 address ");
    for(i = 0; i < 7; ++i) {
      printf("%02x%02x:",
             ipaddr.u8[i * 2], ipaddr.u8[i * 2 + 1]);
    }
    printf("%02x%02x\n",
           ipaddr.u8[7 * 2], ipaddr.u8[7 * 2 + 1]);
  }
#endif
}

/*---------------------------------------------------------------------------*/
/**
 * \brief Main routine for the cc2538dk platform
 */
int
main(void)
{
  nvic_init();
  sys_ctrl_init();
  clock_init();
  lpm_init();
  rtimer_init();
  gpio_init();
  ioc_init();

  process_init();

  watchdog_init();

  /*
   * Character I/O Initialisation.
   * When the UART receives a character it will call serial_line_input_byte to
   * notify the core. The same applies for the USB driver.
   *
   * If slip-arch is also linked in afterwards (e.g. if we are a border router)
   * it will overwrite one of the two peripheral input callbacks. Characters
   * received over the relevant peripheral will be handled by
   * slip_input_byte instead
   */
#if UART_CONF_ENABLE
  uart_init();
  uart_set_input(serial_line_input_byte);
#endif

#if USB_SERIAL_CONF_ENABLE
  usb_serial_init();
  usb_serial_set_input(serial_line_input_byte);
#endif

  serial_line_init();

  INTERRUPTS_ENABLE();

  PUTS(CONTIKI_VERSION_STRING);
  PUTS(BOARD_STRING);

  /* Initialise the H/W RNG engine. */
  random_init(0);

  udma_init();

  process_start(&etimer_process, NULL);
  ctimer_init();

  /* Real Time Timer Init */
  rtimer_init();

  set_rime_addr();
  netstack_init();
  cc2538_rf_set_addr(IEEE802154_PANID);

#if UIP_CONF_IPV6
  memcpy(&uip_lladdr.addr, &rimeaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
  process_start(&tcpip_process, NULL);
#endif /* UIP_CONF_IPV6 */

  print_net_info();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  plugz_switch_driver_init();
  process_start(&sensors_process, NULL);

  autostart_start(autostart_processes);

  watchdog_start();
  plugz_triac_turn_on(1);

  while(1) {
    uint8_t r;
    do {
      /* Reset watchdog and handle polls and events */
      watchdog_periodic();

      r = process_run();
    } while(r > 0);

    /* We have serviced all pending events. Enter a Low-Power mode. */
    lpm_enter();
  }
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */