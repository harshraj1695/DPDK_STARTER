#include <stdio.h>
#include <unistd.h>
#include <rte_eal.h>
#include <rte_ring.h>
#include <rte_malloc.h>

#include "common.h"

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    struct rte_ring *ring = rte_ring_create(
        RING_NAME,
        RING_SIZE,
        rte_socket_id(),
        RING_F_SP_ENQ | RING_F_SC_DEQ
    );

    if (!ring)
        rte_exit(EXIT_FAILURE, "Ring create failed\n");

    printf("Primary: ring created\n");

    for (int i = 0; i < 10; i++) {
        int *msg = rte_malloc(NULL, sizeof(int), 0);
        *msg = i;

        while (rte_ring_enqueue(ring, msg) < 0)
            rte_pause();

        printf("Primary: sent %d\n", i);
        sleep(1);
    }

    printf("Primary: done\n");
    return 0;
}
