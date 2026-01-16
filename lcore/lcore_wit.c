#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_launch.h>
#include <rte_cycles.h>

#include <stdio.h>

static int
lcore_main(void *arg)
{
    unsigned id = rte_lcore_id();

    printf(">>> LCORE %u: started work\n", id);

    rte_delay_us_block((300 + id * 100) * 1000); // different delay per core

    printf(">>> LCORE %u: finished work\n", id);

    return id * 10;   // RETURN VALUE
}

int
main(int argc, char **argv)
{
    rte_eal_init(argc, argv);

    printf("Launching on all lcores...\n");

    rte_eal_mp_remote_launch(lcore_main, NULL, CALL_MAIN);

    unsigned id;
    RTE_LCORE_FOREACH_WORKER(id) {

        printf("Waiting for lcore %u...\n", id);

        int ret = rte_eal_wait_lcore(id);

        printf("<<< LCORE %u returned %d\n", id, ret);
    }

    printf("Main core finished waiting.\n");
    return 0;
}
