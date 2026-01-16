#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

int init_eal(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    printf("EAL initialized.\n");
    return ret;
}
