APP = lcore_wit

# Use pkg-config to get correct flags for your DPDK installation
PKGCONF = $(shell pkg-config --cflags --libs libdpdk)

all: $(APP)

$(APP): lcore_wit.c
	$(CC) lcore_wit.c -o $(APP) $(PKGCONF)

clean:
	rm -f $(APP)

