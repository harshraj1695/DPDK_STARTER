#include "pipeline.h"
#include <rte_ring.h>
#include <rte_mbuf.h>

extern struct rte_ring *ring_rx_to_parse;
extern struct rte_ring *ring_parse_to_tx;
extern volatile int force_quit;
int parser_loop(void *arg)
{
    printf("Parser thread started on lcore %u\n", rte_lcore_id());
    parser_count=0;
    while (!force_quit) {

        struct rte_mbuf *pkts[BURST_SIZE];
        uint16_t n = rte_ring_dequeue_burst(ring_rx_to_parse,
                                            (void **)pkts,
                                            BURST_SIZE,
                                            NULL);

        if (n > 0) {
            printf("Parser: got %u packets\n", n);
            parser_count += n;
            //  Now forward pkts to TX ring 
            uint16_t m = rte_ring_enqueue_burst(ring_parse_to_tx,
                                                (void **)pkts,
                                                n,
                                                NULL);

            if (m < n) {
                //  free dropped packets
                for (int i = m; i < n; i++)
                    rte_pktmbuf_free(pkts[i]);
            }
        }
    }

    return 0;
}
