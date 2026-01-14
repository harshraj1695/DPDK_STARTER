#include "pipeline.h"
#include <rte_ethdev.h>
#include <rte_ring.h>

extern struct rte_ring *ring_parse_to_tx;
extern uint16_t port_id;
extern volatile int force_quit;
int tx_loop(void *arg)
{
    printf("TX thread started on lcore %u\n", rte_lcore_id());
    tx_count = 0;
    while (!force_quit) {

        struct rte_mbuf *pkts[BURST_SIZE];
        uint16_t n = rte_ring_dequeue_burst(ring_parse_to_tx,
                                            (void **)pkts,
                                            BURST_SIZE,
                                            NULL);

        if (n > 0) {
          tx_count+= n;
            printf("TX: sending %u packets (total %lu)\n", n, tx_count);

            uint16_t sent = rte_eth_tx_burst(port_id, 0, pkts, n);

            //  free unsent packets 
            for (int i = sent; i < n; i++)
                rte_pktmbuf_free(pkts[i]);
        }
    }

    return 0;
}

