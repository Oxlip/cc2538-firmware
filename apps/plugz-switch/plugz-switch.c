/**
 * \file
 *      PlugZ switch COAP Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "frame802154.h"
#include "lib/sensors.h"
#include "button-sensor.h"
#include "driver.h"

#include "erbium.h"
#include "er-coap-13.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/*
 * Resources are defined by the RESOURCE macro.
 * Signature: resource name, the RESTful methods it handles, and its URI path (omitting the leading slash).
 */
RESOURCE(helloworld, METHOD_GET, "hello", "title=\"Hello world: ?len=0..\";rt=\"Text\"");

/*
 * A handler function named [resource name]_handler must be implemented for each RESOURCE.
 * A buffer for the response payload is provided through the buffer pointer. Simple resources can ignore
 * preferred_size and offset, but must respect the REST_MAX_CHUNK_SIZE limit for the buffer.
 * If a smaller block size is requested for CoAP, the REST framework automatically splits the data.
 */
void
helloworld_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /* Some data that has the length up to REST_MAX_CHUNK_SIZE. For more, see the chunk resource. */
  char const * const message = "Hello World!";
  int length = 12; /*           |<-------->| */

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

PROCESS(rest_server_example, "Erbium Example Server");
AUTOSTART_PROCESSES(&rest_server_example);

static void
button_press_handler(int button_number)
{
   static int buttons_state = 0; //bitmap of button state
   int is_on = buttons_state & (1 << button_number);

   PRINTF("Switch %d pressed turning %s Triac %d\n",
           button_number, is_on ? "off" : "on"  , button_number);
   if (is_on) {
      plugz_triac_turn_off(button_number);
      buttons_state &= ~(1 << button_number);
   } else {
      plugz_triac_turn_on(button_number);
      buttons_state |= 1 << button_number;
   }
}

PROCESS_THREAD(rest_server_example, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("Starting Erbium PlugZ-Switch Server\n");

  PRINTF("RF channel: %u\n", CC2538_RF_CONF_CHANNEL);
  PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine();

  /* Activate the application-specific resources. */
  rest_activate_resource(&resource_helloworld);

  /* Define application-specific events here. */
  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER) {
       PRINTF("Timer event - and we havent configured one\n");
    } else if (ev == sensors_event) {
      if (data == &button1_sensor) {
         button_press_handler(0);
      } else if (data == &button2_sensor) {
         button_press_handler(1);
      } else if (data == &button3_sensor) {
         button_press_handler(2);
      } else if (data == &button4_sensor) {
         button_press_handler(3);
      }
    }
  } /* while (1) */

  PROCESS_END();
}
