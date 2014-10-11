#ifndef __PROJECT_ASTRAL_FIRMWARE_CONF_H__
#define __PROJECT_ASTRAL_FIRMWARE_CONF_H__

/* Some platforms have weird includes. */
#undef IEEE802154_CONF_PANID

/* Increase rpl-border-router IP-buffer when using more than 64. */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE           256

/* The IP buffer size must fit all other hops, in particular the border router. */
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE          1280

/* Multiplies with chunk size, be aware of memory constraints. */
#undef COAP_MAX_OPEN_TRANSACTIONS
#define COAP_MAX_OPEN_TRANSACTIONS    4

#undef COAP_MAX_OBSERVERS
#define COAP_MAX_OBSERVERS            2

#endif
