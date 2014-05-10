#ifndef __PROJECT_PLUGZ_HUB_CONF_H__
#define __PROJECT_PLUGZ_HUB_CONF_H__

#ifndef UIP_FALLBACK_INTERFACE
#define UIP_FALLBACK_INTERFACE      rpl_interface
#endif

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE        1280
#endif

/* Increase rpl-border-router IP-buffer when using more than 64. */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE    32

#endif /* __PROJECT_PLUGZ_HUB_CONF_H__ */
