#include <stdio.h>
#include <rte_ethdev.h>

/* Declare functions implemented in other .c files */
 
int init_eal(int argc, char **argv);   // extern keyword is not necessary here it is already extern by default
extern struct rte_mempool* create_mempool(void);
extern void init_port(uint16_t port_id, struct rte_mempool *mp);
extern void run_rx_tx_loop(uint16_t port_id);

int main(int argc, char **argv)
{
    init_eal(argc, argv);

    uint16_t nb_ports = rte_eth_dev_count_avail();
    printf("Ports available: %u\n", nb_ports);

    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No ports found!\n");

    uint16_t port_id = 0;

    struct rte_mempool *mp = create_mempool();

    init_port(port_id, mp);

    run_rx_tx_loop(port_id);

    return 0;
}
