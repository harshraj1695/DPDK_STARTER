APP = rx_callback_example
SRCS = simple.c

PKGCONF = pkg-config
DPDK_CFLAGS = $(shell $(PKGCONF) --cflags libdpdk)
DPDK_LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

CFLAGS += -O3 -march=native $(DPDK_CFLAGS)
LDFLAGS += $(DPDK_LDFLAGS)

all: $(APP)

$(APP): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(APP) $(LDFLAGS)

clean:
	rm -f $(APP)
