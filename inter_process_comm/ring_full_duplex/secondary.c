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

    //  Wait until primary creates it
    while (!ring) {
        ring = rte_ring_lookup(RING_NAME);
        sleep(1);
    }

    printf("Secondary: ring1 found\n");

    struct rte_ring *ring2 = NULL;
    while (!ring2) {
        ring2 = rte_ring_lookup(RING_NAME2);
        sleep(1);
    }

    printf("Secondary: ring2 found\n");

   for (;;) {
    void *msg;

    while (rte_ring_dequeue(ring, &msg) < 0)
        rte_pause();

    int *val = msg;

    if (*val == -1) {
        rte_free(val);
        printf("Secondary: exit\n");
        break;
    }

    printf("Secondary: received %d\n", *val);

    int result = (*val) * 10;
    rte_free(val);

    int *reply = rte_malloc(NULL, sizeof(int), 0);
    *reply = result;

    while (rte_ring_enqueue(ring2, reply) < 0)
        rte_pause();

    printf("Secondary: sent %d\n", result);
}

}
