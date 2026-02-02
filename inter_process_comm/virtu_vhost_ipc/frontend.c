#include <stdio.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#define NB_MBUF 8192
#define MBUF_CACHE 256

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    uint16_t port_id = 0;

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

    rte_eth_dev_configure(port_id, 1, 1, NULL);
    rte_eth_rx_queue_setup(port_id, 0, 1024,
                           rte_socket_id(), NULL, mp);
    rte_eth_tx_queue_setup(port_id, 0, 1024,
                           rte_socket_id(), NULL);
    rte_eth_dev_start(port_id);

    printf("Frontend: virtio-user port %u started\n", port_id);

    struct rte_mbuf *tx[1];
    struct rte_mbuf *rx[32];

    while (1) {
        /* allocate packet */
        tx[0] = rte_pktmbuf_alloc(mp);
        char *data = rte_pktmbuf_append(tx[0], 64);
        snprintf(data, 64, "hello from frontend");

        rte_eth_tx_burst(port_id, 0, tx, 1);

        uint16_t n = rte_eth_rx_burst(port_id, 0, rx, 32);
        for (int i = 0; i < n; i++) {
            printf("Frontend: got echo packet\n");
            rte_pktmbuf_free(rx[i]);
        }

        sleep(1);
    }
}
