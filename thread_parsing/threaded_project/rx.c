#include <rte_ring.h>
#include <rte_ethdev.h>
#include "pipeline.h"

extern volatile int force_quit;
extern uint16_t port_id;

int rx_loop(void *arg)
{
    printf("RX thread started\n");
    rx_count = 0;
    while (!force_quit) {

        struct rte_mbuf *pkts[BURST_SIZE];
        uint16_t n = rte_eth_rx_burst(port_id, 0, pkts, BURST_SIZE);

        if (n > 0) {
            rx_count += n;

            for (int i = 0; i < n; i++)
                rte_ring_enqueue(ring_rx_to_parse, pkts[i]);

            printf("RX: received %u packets (total %lu)\n", n, rx_count);
        }

        if (rx_count >= 10) {
            printf("RX reached limit. Stopping...\n");
            force_quit = 1;
            break;
        }
    }

    return 0;
}
