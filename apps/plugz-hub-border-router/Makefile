
all: plugz-hub

CONTIKI=../..

WITH_COAP=13
WITH_UIP6=1
UIP_CONF_IPV6=1
#CFLAGS+= -DUIP_CONF_IPV6_RPL
CFLAGS += -DUIP_CONF_IPV6_RPL=1
CFLAGS += -DUIP_CONF_IPV6=1

#linker optimizations
SMALL=1


PROJECT_SOURCEFILES += slip-bridge.c

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS += -DWEBSERVER=1
CFLAGS += -DWITH_COAP=13
CFLAGS += -DREST=coap_rest_implementation
#CFLAGS += -DUIP_CONF_TCP=0

APPS += er-coap-13
APPS += erbium
APPS += rplinfo

ifeq ($(PREFIX),)
 PREFIX = aaaa::1/64
endif

include $(CONTIKI)/Makefile.include

$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 $(PREFIX)

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 $(PREFIX)
