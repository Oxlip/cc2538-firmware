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
  /* TODO - Calculate the uptime somehow */
  char const * const message = "0";
  const int length = strlen(message);

  memcpy(buffer, message, length);
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


/*-----------------Main Loop / Process -------------------------*/

PROCESS(usense_coap_server, "uSense CoAP server");
PROCESS(periodic_timer_process, "Peridic timer process for testing");
AUTOSTART_PROCESSES(&usense_coap_server, &periodic_timer_process);

PROCESS_THREAD(usense_coap_server, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("Starting uSense CoAP Server(%s %s)\n", __DATE__, __TIME__);

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
