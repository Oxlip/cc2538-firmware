/**
 * \file
 *      PlugZ switch CoAP Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "frame802154.h"
#include "lib/sensors.h"
#include "button-sensor.h"
#include "cc2538-rf.h"
#include "driver.h"
#include "dimmer.h"

#include "er-coap-13.h"
#include "erbium.h"
#include "rplinfo.h"
#include "ota-update.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*
 * etag for COAP, used all the time, so defining it here.
 * TODO: Find the right place for this
 */
static uint8_t coap_etag = 0;
#define MAX_PLUGZ_PAYLOAD 64+1

extern void i2c_test();
extern double plugz_read_internal_voltage();
/*
 * A helper function to dump all sensor information.
 */
static inline void
print_sensor_information()
{
#if USING_CC2538DK
  PRINTF("internal voltage %d\n", (int)plugz_read_internal_voltage());
  return;
#else
  float current_ma, temperature;
  temperature = plugz_read_temperature_sensor_value();
  current_ma = plugz_read_current_sensor_value();
  PRINTF("Internal Vdd=%dmV Current = %dmA(%dW) Temp = %dC\n",
         (int)plugz_read_internal_voltage(),
         (int)current_ma,
         (int)(current_ma * 220) / 1000,
         (int)temperature);
#endif
}

/*
 * Handle button press event. When user presses a switch it is generated as an
 * interrupt which is handled by an ISR(look at platform/plugz-switch/) which
 * generates an button_event. This button event is handled by the main
 * loop(plugz_coap_server).
 */
static void
handle_button_press(int button_number)
{
  int dim_percent = 0;
  static int btn_press_cnt[] = {0, 0, 0, 0};

  dim_percent = 100 - ((++btn_press_cnt[button_number] % 5) * 25);

  printf("Button pressed %d dimming to %d\n", button_number, dim_percent);
  if (dim_percent == 0) {
    dimmer_disable(button_number);
  }
  else {
    dimmer_enable(button_number, dim_percent);
  }
}

/*-----------------IPSO Coap Resource definition--Start----------------------*/
/*http://www.ipso-alliance.org/wp-content/media/draft-ipso-app-framework-04.pdf*/

/* Manufacturer: The manufacturer of the device as a string.*/
RESOURCE(coap_dev_mfg, METHOD_GET, "dev/mfg", "title=\"Manufacturer\";rt=\"ipso.dev.mfg\"");
void
coap_dev_mfg_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = "PlugZ";
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
  char const * const message = "PlugZ Switch";
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
   /* TODO - Write code to get serial number from flash */
  char const * const message = "00000AAA";
  const int length = strlen(message);
  const char *url = NULL;
  REST.get_url(request, &url);
  PRINTF("GET: %s\n", url);

  memcpy(buffer, message, length);
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

/* Name: The descriptive or functional name of the device as a string.*/
RESOURCE(coap_dev_n, METHOD_GET, "dev/n", "title=\"Name\";rt=\"ipso.dev.n\"");
void
coap_dev_n_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char const * const message = "PlugZ Switch - Controls the switches.";
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
  char const * const message = "0"; /* 0-Line, 1-Battery 2-Harvestor */
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
  /* TODO - Calculate this voltage somehow */
  char const * const message = "240v";
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

/* PlugZ Relay, Dimmer and Current Sensor Nodes are defined below */

/* Instantaneous Power: This resource type returns the instantaneous power
 *    of a load as a Decimal value in W.
 */
#define DEFINE_IPSO_COAP_PWR_WATT_NODE(num)                                   \
  RESOURCE(coap_power_watts_##num, METHOD_GET, "dev/pwr/" #num "/w",          \
           "title=\"Instantaneous Power " #num "\";rt=\"ipso.pwr.w\"");       \
                                                                              \
  void                                                                        \
  coap_power_watts_##num##_handler(void* request, void* response,             \
                                   uint8_t *buffer, uint16_t preferred_size,  \
                                   int32_t *offset)                           \
  {                                                                           \
    /* TODO - Fetch from plugz current sensor */                              \
    char const * const message = "0";                                         \
    const int length = strlen(message);                                       \
                                                                              \
    memcpy(buffer, message, length);                                          \
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);             \
    REST.set_header_etag(response, (uint8_t *) &length, 1);                   \
    REST.set_response_payload(response, buffer, length);                      \
  }                                                                           \

/* Cumulative Power: This resource type returns the cumulative power of
 *  a load as a Decimal value in kWh. The value SHOULD be set to zero
 *  on initialization, however the value may be saved and retrieved
 *  from non-volatile memory.
 */
#define DEFINE_IPSO_COAP_PWR_KWATT_NODE(num)                                  \
  RESOURCE(coap_power_kwatts_##num, METHOD_GET, "dev/pwr/" #num "/kw",        \
           "title=\"Cumulative Power " #num "\";rt=\"ipso.pwr.kw\"");         \
                                                                              \
  void                                                                        \
  coap_power_kwatts_##num##_handler(void* request, void* response,            \
                                   uint8_t *buffer, uint16_t preferred_size,  \
                                   int32_t *offset)                           \
  {                                                                           \
    /* TODO - Fetch from plugz current sensor */                              \
    char const * const message = "0";                                         \
    const int length = strlen(message);                                       \
                                                                              \
    memcpy(buffer, message, length);                                          \
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);             \
    REST.set_header_etag(response, (uint8_t *) &length, 1);                   \
    REST.set_response_payload(response, buffer, length);                      \
  }                                                                           \

/*
 * Load Relay: This resource represents a relay attached to the load,
    which can be controlled, the setting of which is a Boolean value
    (1,0). A GET on the resource returns the current state of the
    relay, and a PUT on the resource sets a new state.
 */
#define DEFINE_IPSO_COAP_RELAY_NODE(num)                                      \
  RESOURCE(coap_power_relay_##num, METHOD_GET | METHOD_PUT, "dev/pwr/" #num "/rel",  \
           "title=\"Load Relay" #num "\";rt=\"ipso.pwr.rel\"");               \
                                                                              \
  void                                                                        \
  coap_power_relay_##num##_handler(void* request, void* response,             \
                                   uint8_t *buffer, uint16_t preferred_size,  \
                                   int32_t *offset)                           \
  {                                                                           \
    /* TODO - get relay state */                                              \
    char const * const message = "0";                                         \
    const int length = strlen(message);                                       \
                                                                              \
    memcpy(buffer, message, length);                                          \
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);             \
    REST.set_header_etag(response, (uint8_t *) &length, 1);                   \
    REST.set_response_payload(response, buffer, length);                      \
  }                                                                           \


/*
 * Load Dimmer: This resource represents a power controller attached to
 *  the load, which can be controlled as a % between 0-100. A GET on
 *  the resource returns the current state, and a PUT on the resource
 *  sets a new state.
 */
#define DEFINE_IPSO_COAP_DIMMER_NODE(num)                                     \
  RESOURCE(coap_power_dimmer_##num, METHOD_GET | METHOD_PUT, "dev/pwr/" #num "/dim", \
           "title=\"Load Dimmer" #num "\";rt=\"ipso.pwr.dim\"");              \
                                                                              \
  void                                                                        \
  coap_power_dimmer_##num##_handler(void* request, void* response,            \
                                   uint8_t *buffer, uint16_t preferred_size,  \
                                   int32_t *offset)                           \
  {                                                                           \
     uint8_t method = REST.get_method_type(request);                          \
     const char *url = NULL; \
                                                                              \
     REST.set_header_content_type(response, REST.type.TEXT_PLAIN);            \
     REST.set_header_etag(response, &coap_etag, 1);                           \
     REST.get_url(request, &url);                                             \
                                                                              \
     if (method & METHOD_GET)                                                 \
     {                                                                        \
        PRINTF("GET: 0x%x %s\n", method, url);                                \
        REST.set_response_payload(response, buffer, snprintf((char *)buffer,  \
                 MAX_PLUGZ_PAYLOAD, "%d", dimmer_config[num].percent));       \
     }                                                                        \
     else                                                                     \
     {                                                                        \
        char *incoming = NULL;                                             \
        int len = 0, percent = 0;                                             \
                                                                              \
        len = REST.get_request_payload(request, (const uint8_t **) &incoming);\
        PRINTF("PUT :len = %d , percent  = %s", len, (char *)incoming);       \
                                                                              \
        percent = (int)atoi(incoming);                                        \
        if(percent < 0 || percent > 100)                                      \
        {                                                                     \
           REST.set_response_status(response, REST.status.BAD_REQUEST);       \
           REST.set_response_payload(response, buffer, snprintf((char *)buffer,  \
                    MAX_PLUGZ_PAYLOAD, "Invalid: try between 0 and 100\n"));  \
        }                                                                     \
        else {                                                                \
           if(percent == 0)                                                   \
           {                                                                  \
              dimmer_disable(num);                                            \
              REST.set_response_status(response, REST.status.CHANGED);        \
              REST.set_response_payload(response, buffer, snprintf((char *)buffer,  \
                       MAX_PLUGZ_PAYLOAD, "Dimmer disabled on " #num));       \
           }                                                                  \
           else                                                               \
           {                                                                  \
           REST.set_response_status(response, REST.status.CHANGED);           \
              REST.set_response_payload(response, buffer, snprintf((char *)buffer,  \
                       MAX_PLUGZ_PAYLOAD,  "Dimmer percentage %d\n",          \
                       percent));                                             \
              dimmer_enable(num, percent);                                    \
           }                                                                  \
        }                                                                     \
     }                                                                        \
  }


/* Helper macro to define Power/Relay/Dimmer resources for all 4 switches */
#define DEFINE_IPSO_COAP_PWR_NODE(num)                                        \
  DEFINE_IPSO_COAP_PWR_WATT_NODE(num);                                        \
  DEFINE_IPSO_COAP_PWR_KWATT_NODE(num);                                       \
  DEFINE_IPSO_COAP_RELAY_NODE(num);                                           \
  DEFINE_IPSO_COAP_DIMMER_NODE(num)                                           \

/* Define all 4 switch nodes */
DEFINE_IPSO_COAP_PWR_NODE(0);
DEFINE_IPSO_COAP_PWR_NODE(1);
DEFINE_IPSO_COAP_PWR_NODE(2);
DEFINE_IPSO_COAP_PWR_NODE(3);

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

PROCESS(plugz_coap_server, "PlugZ switch CoAP server");
PROCESS(periodic_timer_process, "Peridic timer process for testing");
AUTOSTART_PROCESSES(&plugz_coap_server, &periodic_timer_process);

PROCESS_THREAD(plugz_coap_server, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("Starting PlugZ-Switch CoAP Server(%s %s)\n", __DATE__, __TIME__);

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
  rest_activate_resource(&resource_coap_power_dimmer_0);
  rest_activate_resource(&resource_coap_power_dimmer_1);
  rest_activate_resource(&resource_coap_power_dimmer_2);
  rest_activate_resource(&resource_coap_power_dimmer_3);

#define ACTIVATE_IPSO_COAP_PWR_NODE(num)                         \
  rest_activate_resource(&resource_coap_power_watts_##num);      \
  rest_activate_resource(&resource_coap_power_kwatts_##num);     \
  rest_activate_resource(&resource_coap_power_relay_##num);      \
  rest_activate_resource(&resource_coap_power_dimmer_##num);     \

  ACTIVATE_IPSO_COAP_PWR_NODE(0);
  ACTIVATE_IPSO_COAP_PWR_NODE(1);
  ACTIVATE_IPSO_COAP_PWR_NODE(2);
  ACTIVATE_IPSO_COAP_PWR_NODE(3);

  rplinfo_activate_resources();
  rest_activate_resource(&resource_coap_radio);

   ota_update_enable();

  /* Handle events */
  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER) {
       PRINTF("Timer event - and we havent configured one\n");
    } else {
      if (ev == sensors_event) {
        if (data == &button1_sensor) {
           handle_button_press(0);
        } else if (data == &button2_sensor) {
           handle_button_press(1);
        } else if (data == &button3_sensor) {
           handle_button_press(2);
        } else if (data == &button4_sensor) {
           handle_button_press(3);
        }
      }
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
