#include <rte_lcore.h>
#include <rte_per_lcore.h>

RTE_DEFINE_PER_LCORE(int, my_counter);

int lcore_main(void *arg) {
    unsigned id = rte_lcore_id();

    RTE_PER_LCORE(my_counter)++;

    printf("Core %u: counter = %d\n", id, RTE_PER_LCORE(my_counter));

    return 0;
}

int main(int argc, char **argv) {
    rte_eal_init(argc, argv);

    rte_eal_mp_remote_launch(lcore_main, NULL, CALL_MAIN);

    rte_eal_mp_wait_lcore();
    return 0;
}
