#include <rte_eal.h>
#include <rte_malloc.h>
#include <rte_ring.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

int main(int argc, char **argv) {
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "EAL init failed\n");

  struct rte_ring *ring = rte_ring_create(RING_NAME, RING_SIZE, rte_socket_id(),
                                          0);

  if (!ring)
    rte_exit(EXIT_FAILURE, "Ring create failed\n");

  printf("Primary: ring1 created\n");

  struct rte_ring *ring2 = rte_ring_create(
      RING_NAME2, RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

  if (!ring2)
    rte_exit(EXIT_FAILURE, "Ring2 create failed\n");

  printf("Primary: ring2 created\n");

  for (int i = 0; i < 10; i++) {
    int *msg = rte_malloc(NULL, sizeof(int), 0);
    *msg = i;

    while (rte_ring_enqueue(ring, msg) < 0)
      rte_pause();

    printf("Primary: sent %d\n", i);

    void *reply;
    while (rte_ring_dequeue(ring2, &reply) < 0)
      rte_pause();

    int *val = reply;
    printf("Primary: received %d\n", *val);
    rte_free(val);
  }

  //  tell secondary to stop
  int *stop = rte_malloc(NULL, sizeof(int), 0);
  *stop = -1;
  rte_ring_enqueue(ring, stop);

  printf("Primary: done\n");

  return 0;
}
