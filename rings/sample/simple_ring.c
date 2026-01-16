#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_ring.h>

#define RING_SIZE 1024
#define BURST_SIZE 1

static volatile int keep_running = 1;

static void handle_signal(int sig) {
    printf("Stoping the application...\n");
    keep_running = 0;
}

// Producer thread 
static int producer(void *arg) {
    struct rte_ring *ring = arg;
    uint64_t counter = 0;

    while (keep_running) {
        uint64_t *data = malloc(sizeof(uint64_t));
        *data = counter++;

        int ret = rte_ring_enqueue(ring, data);
        if (ret == 0) {
            printf("[PROD] Enqueued: %lu\n", *data);
        } else {
            printf("[PROD] Ring full!\n");
            free(data);
        }

        rte_delay_ms(500);
    }

    return 0;
}

// Consumer thread 
static int consumer(void *arg) {
    struct rte_ring *ring = arg;

    while (keep_running) {
        void *ptr = NULL;

        int ret = rte_ring_dequeue(ring, &ptr);
        if (ret == 0) {
            uint64_t *data = (uint64_t *)ptr;
            printf("    [CONS] Dequeued: %lu\n", *data);
            free(data);
        } else {
            // printf("    [CONS] Ring empty\n");
        }

        rte_delay_ms(300);
    }

    return 0;
}

int main(int argc, char **argv) {
    int ret;
    unsigned lcore_id;

    signal(SIGINT, handle_signal);

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error initializing EAL\n");

    argc -= ret;
    argv += ret;

    // Create ring
    struct rte_ring *ring = rte_ring_create(
        "my_ring",
        RING_SIZE,
        rte_socket_id(),
        RING_F_SP_ENQ | RING_F_SC_DEQ     // single producer/consumer
    );

    if (ring == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create ring\n");

    printf("Ring created. Launching threads…\n");

 
    rte_eal_remote_launch(producer, ring, 1);
    rte_eal_remote_launch(consumer, ring, 2);

    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_wait_lcore(lcore_id);
    }

    return 0;
}
