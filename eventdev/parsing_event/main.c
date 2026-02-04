#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include <rte_eventdev.h>
#include <rte_event_eth_rx_adapter.h>
#include <rte_event_eth_tx_adapter.h>

#define EVENT_DEV_ID 0

#define RX_QUEUE_ID 0
#define TX_QUEUE_ID 1

#define NB_QUEUES 2
#define NB_PORTS  2

#define NB_MBUFS 8192
#define MBUF_CACHE 256
#define BURST 32

static struct rte_mempool *mbuf_pool;

//  * RX adapter configuration callback (matches YOUR DPDK header exactly)
static int
rx_adapter_conf_cb(uint8_t dev_id,
                   uint8_t adapter_id,
                   struct rte_event_eth_rx_adapter_conf *conf,
                   void *arg)
{
    uint8_t event_port_id = *(uint8_t *)arg;

    // Tell adapter to use an EXISTING event port
    conf->event_port_id = event_port_id;

    // event_sw requires external port model
    // conf->use_internal_port = 0;

    return 0;
}


 // Worker
static int
event_worker(void *arg)
{
    struct rte_event ev;
    uint8_t event_port_id = 0;

    printf("Parser running on lcore %u\n", rte_lcore_id());

    while (1) {
        if (!rte_event_dequeue_burst(
                EVENT_DEV_ID, event_port_id, &ev, 1, 0))
            continue;

        struct rte_mbuf *m = ev.event_ptr;
        struct rte_ether_hdr *eth =
            rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

        if (eth->ether_type ==
            rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {

            struct rte_ipv4_hdr *ip =
                (struct rte_ipv4_hdr *)(eth + 1);

            printf("Parsed IPv4 src=%08x dst=%08x\n",
                   rte_be_to_cpu_32(ip->src_addr),
                   rte_be_to_cpu_32(ip->dst_addr));
        }

        ev.queue_id = TX_QUEUE_ID;

        rte_event_enqueue_burst(
            EVENT_DEV_ID, event_port_id, &ev, 1);
    }
    return 0;
}

// Main
int
main(int argc, char **argv)
{
    int ret;
    uint16_t eth_port_id = 0;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    printf("Ethdev ports available: %u\n",
           rte_eth_dev_count_avail());

    //  Mempool 
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NB_MBUFS,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "mbuf pool create failed\n");

    // Ethdev (net_null) 

    struct rte_eth_conf eth_conf = {0};

    rte_eth_dev_configure(eth_port_id, 1, 1, &eth_conf);

    rte_eth_rx_queue_setup(
        eth_port_id, RX_QUEUE_ID, 1024,
        rte_eth_dev_socket_id(eth_port_id),
        NULL, mbuf_pool);

    rte_eth_tx_queue_setup(
        eth_port_id, 0, 1024,
        rte_eth_dev_socket_id(eth_port_id),
        NULL);

    rte_eth_dev_start(eth_port_id);
    rte_eth_promiscuous_enable(eth_port_id);

    // Event device 
    struct rte_event_dev_config dev_conf = {
        .nb_event_queues = NB_QUEUES,
        .nb_event_ports  = NB_PORTS,
        .nb_event_queue_flows = 1024,
        .nb_event_port_dequeue_depth = BURST,
        .nb_event_port_enqueue_depth = BURST,
        .nb_events_limit = 4096,
    };

    rte_event_dev_configure(EVENT_DEV_ID, &dev_conf);

    struct rte_event_queue_conf qconf = {
        .schedule_type = RTE_SCHED_TYPE_ATOMIC,
        .nb_atomic_flows = 1024,
        .nb_atomic_order_sequences = 1024,
    };

    rte_event_queue_setup(EVENT_DEV_ID, RX_QUEUE_ID, &qconf);
    rte_event_queue_setup(EVENT_DEV_ID, TX_QUEUE_ID, &qconf);

    struct rte_event_port_conf pconf;

    // Event port 0 -> worker
    rte_event_port_default_conf_get(EVENT_DEV_ID, 0, &pconf);
    rte_event_port_setup(EVENT_DEV_ID, 0, &pconf);

    // Event port 1 -> RX adapter
    rte_event_port_default_conf_get(EVENT_DEV_ID, 1, &pconf);
    rte_event_port_setup(EVENT_DEV_ID, 1, &pconf);

    uint8_t queues[] = { RX_QUEUE_ID, TX_QUEUE_ID };
    rte_event_port_link(EVENT_DEV_ID, 0, queues, NULL, 2);

    rte_event_dev_start(EVENT_DEV_ID);

    // RX adapter (external port model)

    uint8_t rx_adapter_id = 0;
    uint8_t rx_event_port_id = 1;

    ret = rte_event_eth_rx_adapter_create_ext(
        rx_adapter_id,
        EVENT_DEV_ID,
        rx_adapter_conf_cb,
        &rx_event_port_id);

    if (ret < 0)
        rte_exit(EXIT_FAILURE, "RX adapter create failed\n");

    struct rte_event_eth_rx_adapter_queue_conf rxq_conf = {0};

    rxq_conf.ev.queue_id   = RX_QUEUE_ID;
    rxq_conf.ev.sched_type = RTE_SCHED_TYPE_ATOMIC;
    rxq_conf.ev.priority  = RTE_EVENT_DEV_PRIORITY_NORMAL;

    rte_event_eth_rx_adapter_queue_add(
        rx_adapter_id,
        eth_port_id,
        RX_QUEUE_ID,
        &rxq_conf);

    rte_event_eth_rx_adapter_start(rx_adapter_id);



    printf("RX adapter started, waiting for packets...\n");

    //  TX adapter 
    uint8_t tx_adapter_id = 0;

    rte_event_eth_tx_adapter_create(
        tx_adapter_id,
        EVENT_DEV_ID,
        NULL);

    rte_event_eth_tx_adapter_queue_add(
        tx_adapter_id,
        eth_port_id,
        TX_QUEUE_ID);

    rte_event_eth_tx_adapter_start(tx_adapter_id);


// Launch worker -

    unsigned int lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_remote_launch(
            event_worker, NULL, lcore_id);
        break;
    }

    rte_eal_mp_wait_lcore();
    return 0;
}
