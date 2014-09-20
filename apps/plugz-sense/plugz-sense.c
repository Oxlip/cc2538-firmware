/**
 * \file
 *      uSense CoAP Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "frame802154.h"
#include "lib/sensors.h"
#include "button-sensor.h"
#include "leds.h"
#include "cc2538-rf.h"
#include "driver.h"
#include "adc.h"

#include "er-coap-13.h"
#include "erbium.h"
#include "rplinfo.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define MAX_USENSE_PAYLOAD  64+1
#define COMPANY_NAME        "Astral"
#define PRODUCT_MODEL_NAME  "uSense"

/*
 * A helper function to dump all sensor information.
 */
static inline void
print_sensor_information()
{
  long uptime = clock_seconds();
  int hours, minutes, seconds;
  seconds = uptime % 60;
  minutes = uptime / 60;
  hours = minutes / 60;
  minutes = minutes % 60;
  PRINTF("%02d:%02d:%02d: Internal Vdd=%dmV ", hours, minutes, seconds, (int)get_vdd());
#ifndef USING_CC2538DK
  float lux, temperature;
  int32_t humdity;
  lux = get_ambient_lux();
  read_si7013(&temperature, &humdity);
  PRINTF("temperature = %dC humdity=%d%% lux=%d(%d%%)",
         (int)temperature/1000,
         (int)humdity/1000,
         (int)lux,
         (int)lux_to_pct(lux)
         );
#endif
  PRINTF("\n");
}

/*-----------------IPSO Coap Resource definition--Start----------------------*/
/*http://www.ipso-alliance.org/wp-content/media/draft-ipso-app-framework-04.pdf*/

/* Manufacturer: The manufacturer of the device as a string.*/
RESOURCE(coap_dev_mfg, METHOD_GET, "dev/mfg", "title=\"Manufacturer\";rt=\"ipso.dev.mfg\"");
void
coap_dev_mfg_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = COMPANY_NAME;
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Model: The model of the device as a string. */
RESOURCE(coap_dev_mdl, METHOD_GET, "dev/mdl", "title=\"Model\";rt=\"ipso.dev.mdl\"");
void
coap_dev_mdl_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = PRODUCT_MODEL_NAME;
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Hardware Revision: The version of the hardware of the device as a string.*/
RESOURCE(coap_dev_mdl_hw, METHOD_GET, "dev/mdl/hw", "title=\"Hardware revision\";rt=\"ipso.dev.mdl.hw\"");
void
coap_dev_mdl_hw_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = "0.1";
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Software Version: The version of the software embedded in the device as a string.*/
RESOURCE(coap_dev_mdl_sw, METHOD_GET, "dev/mdl/sw", "title=\"Software revision\";rt=\"ipso.dev.mdl.sw\"");
void
coap_dev_mdl_sw_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = "0.1";
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Serial: The serial number of the device as a string. */
RESOURCE(coap_dev_ser, METHOD_GET, "dev/ser", "title=\"Serial Number\";rt=\"ipso.dev.ser\"");
void
coap_dev_ser_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *url = NULL;
  int length;

  REST.get_url(request, &url);
  PRINTF("GET: %s\n", url);

  length = sprintf((char *)buffer, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], linkaddr_node_addr.u8[2],
    linkaddr_node_addr.u8[3], linkaddr_node_addr.u8[4], linkaddr_node_addr.u8[5],
    linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Name: The descriptive or functional name of the device as a string.*/
RESOURCE(coap_dev_n, METHOD_GET, "dev/n", "title=\"Name\";rt=\"ipso.dev.n\"");
void
coap_dev_n_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = COMPANY_NAME " " PRODUCT_MODEL_NAME;
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Power Supply: The type of power supply as an enumeration Table 1.
    0-Line, 1-Battery 2-Harvestor
 */
RESOURCE(coap_dev_pwr, METHOD_GET, "dev/pwr/0", "title=\"Power Source\";rt=\"ipso.dev.pwr\"");
void
coap_dev_pwr_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = "1"; /* 0-Line, 1-Battery 2-Harvestor */
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Power Supply Voltage: The supply level of the device in Volts.*/
RESOURCE(coap_dev_pwr_v, METHOD_GET, "dev/pwr/0/v", "title=\"Power Voltage\";rt=\"ipso.dev.pwr.v\"");
void
coap_dev_pwr_v_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /* TODO - measure this voltage using the ADC */
  char const * const message = "5v";
  const int length = strlen(message);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Uptime: The number of seconds that have elapsed since the device was turned on. */
RESOURCE(coap_uptime, METHOD_GET, "dev/uptime", "title=\"Uptime\";rt=\"ipso.dev.uptime\"");
void
coap_uptime_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  int length = sprintf((char *)buffer, "%ld" , clock_seconds()) ;

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Returns the reading of the rssi/lqi from radio sensor */
RESOURCE(coap_radio, METHOD_GET, "debug/radio", "title=\"RADIO\";rt=\"RadioSensor\"");

void
coap_radio_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  int length;

  length = snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'rssi':%d}", cc2538_rf_read_rssi());
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

#if DEBUG

RESOURCE(coap_led,
	 METHOD_GET | METHOD_POST | METHOD_PUT,
	 "dev/led",
	 "title=\"Led\";rt=\"debug.led\"");
void
coap_led_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  uint8_t method = REST.get_method_type(request);

  leds_blink();
  if (method & METHOD_GET) {
    unsigned char leds = leds_get();
    char         *bufIter = (char*)buffer;
    int           lengthTot = 0;
    int           length;

    lengthTot = sprintf((char *)bufIter,
			 "You can switch the light with a POST"
			 " request. Ex: 'red on' or 'green off'\n"
			 "Current status: ");
    bufIter += lengthTot;
#define USENSE_PRINT_LED(color, colorStr)				\
    if (leds & LEDS_##color) {						\
      length = snprintf((char *)bufIter,				\
			preferred_size - lengthTot, colorStr);		\
      lengthTot += length;						\
      bufIter += length;						\
    }
    USENSE_PRINT_LED(RED, " red ")
    USENSE_PRINT_LED(GREEN, " green ")
    USENSE_PRINT_LED(YELLOW, " yellow ")
    USENSE_PRINT_LED(ORANGE, " orange ")
#undef USENSE_PRINT_LED

    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_header_etag(response, (uint8_t *) &lengthTot, 1);
    REST.set_response_payload(response, buffer, lengthTot);

  } else if (method & (METHOD_POST)) {

    coap_packet_t  *pkt = (coap_packet_t*)request;
    const char     *payload = (const char*)pkt->payload;
    unsigned char   led = 0;
    unsigned int    led_length;

    printf("POST payload: %.*s\n", pkt->payload_len, payload);

    if (!strncmp(payload, "red", MIN(3, pkt->payload_len))) {
      led = LEDS_RED;
      led_length = 3;
    } else if (!strncmp(payload, "green", MIN(5, pkt->payload_len))) {
      led = LEDS_GREEN;
      led_length = 5;
    } else if (!strncmp(payload, "yellow", MIN(6, pkt->payload_len))) {
      led = LEDS_YELLOW;
      led_length = 6;
    } else if (!strncmp(payload, "orange", MIN(6, pkt->payload_len))) {
      led = LEDS_ORANGE;
      led_length = 6;
    }
    led_length++;
    if (led) {
      if (!strncmp(payload + led_length, "on",
		   MIN(pkt->payload_len - led_length, 2))) {
	leds_on(led);
      }
      else if (!strncmp(payload + led_length, "off",
			MIN(pkt->payload_len - led_length, 3))) {
	leds_off(led);
      }
      else {
	printf("What do you wanna do with this led?\n");
	// TODO: handle error
      }
    } else {
      printf("unsupported led color\n");
      // TODO: handle error
    }
    REST.set_response_status(response, REST.status.CREATED);
    REST.set_header_location(response, "/dev/led");

  } else {
    // Error: Unhandled method
  }
}

#endif /* !DEBUG */

/*-----------------Main Loop / Process -------------------------*/

PROCESS(usense_coap_server, "uSense CoAP server");
PROCESS(periodic_timer_process, "Peridic timer process for testing");
AUTOSTART_PROCESSES(&usense_coap_server, &periodic_timer_process);

PROCESS_THREAD(usense_coap_server, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("%s %s %s %s\n", COMPANY_NAME, PRODUCT_MODEL_NAME, __DATE__, __TIME__);

  PRINTF("RF channel: %u\n", CC2538_RF_CONF_CHANNEL);
  PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine();

  /* Activate the CoAP resources. */
  rest_activate_resource(&resource_coap_dev_mfg);
  rest_activate_resource(&resource_coap_dev_mdl);
  rest_activate_resource(&resource_coap_dev_mdl_hw);
  rest_activate_resource(&resource_coap_dev_mdl_sw);
  rest_activate_resource(&resource_coap_dev_ser);
  rest_activate_resource(&resource_coap_dev_n);
  rest_activate_resource(&resource_coap_dev_pwr);
  rest_activate_resource(&resource_coap_dev_pwr_v);
  rest_activate_resource(&resource_coap_uptime);
  rest_activate_resource(&resource_coap_radio);
#if DEBUG
  rest_activate_resource(&resource_coap_led);
#endif /* !DEBUG */

  rplinfo_activate_resources();

  leds_on(LEDS_GREEN);

  /* Handle events */
  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER) {
       PRINTF("Timer event - and we havent configured one\n");
    }
  } /* while (1) */

  PROCESS_END();
}


static struct etimer et;
PROCESS_THREAD(periodic_timer_process, ev, data)
{
   PROCESS_BEGIN();

   etimer_set(&et, CLOCK_SECOND * 2);
   while(1) {
      PROCESS_WAIT_EVENT();
      if(ev == PROCESS_EVENT_TIMER) {
         print_sensor_information();
         etimer_reset(&et);
      }
   }

   PROCESS_END();;
}
