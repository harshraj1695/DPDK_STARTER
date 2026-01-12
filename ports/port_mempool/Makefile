APP = dpdk_tun_app

SRCS = main.c eal_init.c mempool.c port.c rx_tx.c

CFLAGS += -O3 -I/usr/local/include -include rte_config.h -march=native
LDFLAGS += -L/usr/local/lib/x86_64-linux-gnu \
           -lrte_eal -lrte_ethdev -lrte_mempool -lrte_mbuf -lrte_kvargs -lrte_bus_vdev

all:
	cc $(SRCS) -o $(APP) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(APP)
