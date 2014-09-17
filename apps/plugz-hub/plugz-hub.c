/**
 * \file
 *      uHub CoAP Server
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "dev/button-sensor.h"
#include "dev/slip.h"
#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "erbium.h"
#include "rplinfo.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define LEDS_ROUTE_ADD (LEDS_GREEN)
#define LEDS_ROUTE_RM  (LEDS_RED)
#define LEDS_DEFRT_ADD (LEDS_GREEN | LEDS_YELLOW)
#define LEDS_DEFRT_RM  (LEDS_RED | LEDS_YELLOW)
#define LEDS_ON_DELAY  (RTIMER_SECOND >> 1)

uint16_t dag_id[] = {0x1111, 0x1100, 0, 0, 0, 0, 0, 0x0011};

static uip_ipaddr_t prefix;
static uint8_t prefix_set;
static struct rtimer rt_dels;

PROCESS(border_router_process, "uHub Border Router process");

AUTOSTART_PROCESSES(&border_router_process);

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTA("\n");
    }
  }
}

void
request_prefix(void)
{
  /* mess up uip_buf with a dirty request... */
  uip_buf[0] = '?';
  uip_buf[1] = 'P';
  uip_len = 2;
  slip_send();
  uip_len = 0;
}

void
set_prefix_64(uip_ipaddr_t *prefix_64)
{
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);
  prefix_set = 1;
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}

/*
 * Callback used to switch off the given leds.
 */
void
rt_leds_off_callback(struct rtimer *t, void *ptr)
{
  leds_off((unsigned char)((unsigned int)ptr));
}


/*
 * This callback will be invoked by the neighbor discovery code.
 */
static void
route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int route_count)
{
  unsigned char leds = 0;

  if (event == UIP_DS6_NOTIFICATION_ROUTE_ADD) {
    PRINTA("ROUTE_ADD");
    leds = LEDS_ROUTE_ADD;
  } else if(event == UIP_DS6_NOTIFICATION_ROUTE_RM) {
    PRINTA("ROUTE_RM");
    leds = LEDS_ROUTE_RM;
  } else if (event == UIP_DS6_NOTIFICATION_DEFRT_ADD) {
    PRINTA("DEFRT_ADD");
    leds = LEDS_DEFRT_ADD;
  } else if(event == UIP_DS6_NOTIFICATION_DEFRT_RM) {
    PRINTA("DEFRT_RM");
    leds = LEDS_DEFRT_RM;
  } else {
    PRINTA("OTHER");
  }
  PRINTA(" ");
  uip_debug_ipaddr_print(ipaddr);
  PRINTA("->");
  uip_debug_ipaddr_print(route);
  PRINTA(" (%d)\n", route_count);

  if (leds) {
    leds_on(leds);
    if (!rtimer_set(&rt_dels,
		    RTIMER_NOW() + LEDS_ON_DELAY,
		    0,
		    rt_leds_off_callback,
		    (void*)((unsigned int)leds))) {
      PRINTF("Can't register rtimer rt_dels\n");
    }
  }

}

PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;
  rpl_dag_t *dag;
  static struct uip_ds6_notification ds6_notification_node;

  PROCESS_BEGIN();

/* While waiting for the prefix to be sent through the SLIP connection, the future
 * border router can join an existing DAG as a parent or child, or acquire a default
 * router that will later take precedence over the SLIP fallback interface.
 * Prevent that by turning the radio off until we are initialized as a DAG root.
 */
  prefix_set = 0;
  NETSTACK_MAC.off(0);

  PROCESS_PAUSE();

  PRINTF("Starting uHub (%s %s)\n", __DATE__, __TIME__);

  /* Request prefix until it has been received */
  while(!prefix_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_prefix();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)dag_id);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }

  /* Now turn the radio on, but disable radio duty cycling.
   * Since we are the DAG root, reception delays would constrain mesh throughbut.
   */
  NETSTACK_MAC.off(1);

  rest_init_engine();

  print_local_addresses();
  rplinfo_activate_resources();

  /* Register router add/delete notifications so that we can find new devices
   * that being discovered.
   */
  uip_ds6_notification_add(&ds6_notification_node, route_callback);

  while(1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}

