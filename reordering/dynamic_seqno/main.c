#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mbuf_dyn.h>
// #include <rte_exit.h>

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 256
#define BURST_SIZE 32

// Global mempool pointer
static struct rte_mempool *mbuf_pool;

// Offset for dynamic sequence number field
static int seqno_offset = -1;


// Register dynamic field for sequence number
static void register_seqno_dynfield(void)
{
    struct rte_mbuf_dynfield seqno_dynfield = {
        .name  = "example_seq_no",
        .size  = sizeof(uint32_t),
        .align = __alignof__(uint32_t),
    };

    seqno_offset = rte_mbuf_dynfield_register(&seqno_dynfield);

    if (seqno_offset < 0)
        rte_exit(EXIT_FAILURE, "Failed to register dynamic field\n");

    printf("Dynamic field registered. Offset = %d\n", seqno_offset);
}


// Initialize Ethernet port
static int port_init(uint16_t port)
{
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_rings = 1;
    const uint16_t tx_rings = 0;
    int retval;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval < 0)
        return retval;

    retval = rte_eth_rx_queue_setup(
                port,
                0,
                128,
                rte_eth_dev_socket_id(port),
                NULL,
                mbuf_pool);
    if (retval < 0)
        return retval;

    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    printf("Port %u initialized\n", port);
    return 0;
}


// Main packet processing loop
static void lcore_main_loop(void)
{
    struct rte_mbuf *pkts[BURST_SIZE];
    uint32_t seq = 0;

    printf("Starting RX loop...\n");

    while (1) {

        uint16_t nb_rx = rte_eth_rx_burst(0, 0, pkts, BURST_SIZE);

        if (nb_rx == 0)
            continue;

        for (uint16_t i = 0; i < nb_rx; i++) {

            // Assign sequence number to first 10 packets
            if (seq < 10) {

                uint32_t *seqno =
                    RTE_MBUF_DYNFIELD(
                        pkts[i],
                        seqno_offset,
                        uint32_t *);

                *seqno = seq;

                printf("Assigned seq_no = %u\n", *seqno);

                seq++;
            }

            // Free packet after processing
            rte_pktmbuf_free(pkts[i]);
        }

        // Stop after assigning 10 sequence numbers
        if (seq >= 10) {
            printf("Assigned sequence numbers to 10 packets. Exiting.\n");
            break;
        }
    }
}


// Main function
int main(int argc, char **argv)
{
    int ret;

    // Initialize DPDK EAL
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL init\n");

    // Create mbuf mempool
    mbuf_pool = rte_pktmbuf_pool_create(
                    "MBUF_POOL",
                    NUM_MBUFS,
                    MBUF_CACHE_SIZE,
                    0,
                    RTE_MBUF_DEFAULT_BUF_SIZE,
                    rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // Register dynamic sequence field
    register_seqno_dynfield();

    // Initialize port 0
    if (port_init(0) != 0)
        rte_exit(EXIT_FAILURE, "Cannot init port 0\n");

    // Start packet processing
    lcore_main_loop();

    // Stop and close port
    rte_eth_dev_stop(0);
    rte_eth_dev_close(0);

    printf("Application exiting\n");

    return 0;
}
