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

    struct rte_ring *ring = NULL;

    /* Wait until primary creates it */
    while (!ring) {
        ring = rte_ring_lookup(RING_NAME);
        sleep(1);
    }

    printf("Secondary: ring found\n");

    for (;;) {
        void *msg;
        if (rte_ring_dequeue(ring, &msg) == 0) {
            int *val = msg;
            printf("Secondary: received %d\n", *val);
            rte_free(val);
        }
    }
}
