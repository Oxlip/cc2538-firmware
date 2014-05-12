all: plugz-switch

PROJECT_SOURCEFILES += ota-update.c

WITH_COAP=13

# variable for Makefile.include
WITH_UIP6=1

UIP_CONF_IPV6=1

# IPv6 make config disappeared completely
CFLAGS += -DUIP_CONF_IPV6=1

CONTIKI=../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# variable for Makefile.include
CFLAGS += -DUIP_CONF_IPV6_RPL=1

CFLAGS += -DWITH_COAP=13
CFLAGS += -DREST=coap_rest_implementation
CFLAGS += -DUIP_CONF_TCP=0

APPS += er-coap-13
APPS += erbium
APPS += rplinfo

# linker optimizations
SMALL=1

# optional rules to get assembly
#CUSTOM_RULE_C_TO_OBJECTDIR_O = 1
#CUSTOM_RULE_S_TO_OBJECTDIR_O = 1

include $(CONTIKI)/Makefile.include
