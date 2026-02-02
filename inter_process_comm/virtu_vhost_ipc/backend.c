#include <stdio.h>
#include <unistd.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_vhost.h>
#include <rte_mbuf.h>

#define SOCK_PATH "/tmp/vhost.sock"
#define NB_MBUF 8192
#define MBUF_CACHE 256
#define port_id 0
int main(int argc, char **argv)
{
    rte_eal_init(argc, argv);

    unlink(SOCK_PATH);  // IMPORTANT

    if (rte_vhost_driver_register(SOCK_PATH, 0) < 0)
        rte_exit(EXIT_FAILURE, "vhost register failed\n");

    if (rte_vhost_driver_start(SOCK_PATH) < 0)
        rte_exit(EXIT_FAILURE, "vhost start failed\n");

    printf("Backend: vHost server started at %s\n", SOCK_PATH);
    printf("Backend: waiting for frontend...\n");

    /* Wait for virtio-user frontend */
    // while (rte_eth_dev_count_avail() == 0)
    //     sleep(1);

    // uint16_t port_id;
    // RTE_ETH_FOREACH_DEV(port_id) {
    //     printf("Backend: found vhost port %u\n", port_id);
    //     break;
    // }

    struct rte_mempool *mp = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NB_MBUF,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );

    if (!mp)
        rte_exit(EXIT_FAILURE, "mbuf pool failed\n");

    if (rte_eth_dev_configure(port_id, 1, 1, NULL) < 0)
        rte_exit(EXIT_FAILURE, "dev configure failed\n");

    if (rte_eth_rx_queue_setup(port_id, 0, 1024,
                               rte_socket_id(), NULL, mp) < 0)
        rte_exit(EXIT_FAILURE, "rx queue failed\n");

    if (rte_eth_tx_queue_setup(port_id, 0, 1024,
                               rte_socket_id(), NULL) < 0)
        rte_exit(EXIT_FAILURE, "tx queue failed\n");

    if (rte_eth_dev_start(port_id) < 0)
        rte_exit(EXIT_FAILURE, "dev start failed\n");

    printf("Backend: port %u up, echoing packets\n", port_id);

    struct rte_mbuf *pkts[32];

    while (1) {
        uint16_t n = rte_eth_rx_burst(port_id, 0, pkts, 32);
        if (n)
            rte_eth_tx_burst(port_id, 0, pkts, n);
    }
}
