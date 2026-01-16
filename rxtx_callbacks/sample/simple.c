#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

 // Simple RX callback function
static uint16_t
my_rx_callback(uint16_t port,
               uint16_t queue,
               struct rte_mbuf **pkts,
               uint16_t nb_pkts,
               uint16_t mbuf_alloc_failed,   
               void *user_data)
{
    uint64_t *counter = (uint64_t *)user_data;
    *counter += nb_pkts;

    return nb_pkts;
}


int main(int argc, char **argv)
{
    int ret;
    uint16_t port_id = 0;
    uint64_t rx_count = 0;

     // 1. Initialize EAL
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_panic("EAL init failed\n");

     //2. Configure port with 1 RX queue + 1 TX queue

    struct rte_eth_conf port_conf = {0};

    rte_eth_dev_configure(port_id, 1, 1, &port_conf);

    // RX queue setup 
    rte_eth_rx_queue_setup(port_id, 0, 128,
                           rte_eth_dev_socket_id(port_id),
                           NULL, rte_pktmbuf_pool_create(
                               "MBUF_POOL", 8192,
                               256, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                               rte_socket_id()));

    // TX queue setup 
    rte_eth_tx_queue_setup(port_id, 0, 128,
                           rte_eth_dev_socket_id(port_id),
                           NULL);

    // Start the device 
    rte_eth_dev_start(port_id);

     // 3. Install RX callback

    rte_eth_add_rx_callback(port_id, 0, my_rx_callback, &rx_count);

     //4. Run RX loop
    struct rte_mbuf *pkts_burst[32];

    printf("Packet RX loop started...\n");

    while (1) {
        uint16_t nb = rte_eth_rx_burst(port_id, 0, pkts_burst, 32);

        if (nb > 0) {
            printf("Got %u packets (total seen by callback = %lu)\n",
                   nb, rx_count);

            /* Free the packets */
            for (int i = 0; i < nb; i++)
                rte_pktmbuf_free(pkts_burst[i]);
        }
    }

    return 0;
}
