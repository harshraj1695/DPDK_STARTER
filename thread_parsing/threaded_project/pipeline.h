#ifndef PIPELINE_H
#define PIPELINE_H

#include <rte_mbuf.h>
#include <rte_ring.h>
#include <rte_ethdev.h>

#define RX_RING_SIZE   1024
#define TX_RING_SIZE   1024
#define BURST_SIZE     32

extern uint64_t rx_count;
extern uint64_t parser_count;
extern uint64_t tx_count;

extern struct rte_ring *ring_rx_to_parse;
extern struct rte_ring *ring_parse_to_tx;

int rx_loop(void *arg);
int parser_loop(void *arg);
int tx_loop(void *arg);

#endif