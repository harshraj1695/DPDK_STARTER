#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_telemetry.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static int
app_stats_handler(const char *cmd __rte_unused,
                  const char *params __rte_unused,
                  struct rte_tel_data *d)
{
    printf(">>> app_stats_handler CALLED <<<\n");
    printf(">>> cmd: %s, params: %s\n", cmd ? cmd : "(null)", params ? params : "(null)");
    fflush(stdout);

    if (d == NULL) {
        printf(">>> ERROR: d is NULL!\n");
        fflush(stdout);
        return -1;
    }

    rte_tel_data_start_dict(d);
    rte_tel_data_add_dict_string(d, "app", "minimal-telemetry");
    rte_tel_data_add_dict_int(d, "lcore_count", rte_lcore_count());
    rte_tel_data_add_dict_int(d, "main_lcore", rte_get_main_lcore());

    printf(">>> Handler returning 0 (success)\n");
    fflush(stdout);
    return 0;
}

int
main(int argc, char **argv)
{
    int ret;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    printf("EAL initialized\n");

    // Small delay to ensure telemetry is ready
    sleep(1);

    ret = rte_telemetry_register_cmd("/harsh_stats", app_stats_handler,
                                    "Application statistics");

    printf("Telemetry register returned: %d\n", ret);
    if (ret < 0)
        printf("Telemetry register FAILED: %d\n", ret);
    else
        printf("Telemetry register OK: /harsh_stats\n");
    
    // Try to list commands to verify telemetry is working
    printf("\nTry running: python3 telementry.py (with cmd = \"/\")\n");
    printf("to see if /harsh_stats appears in the command list\n\n");

    printf("PID %d running...\n", getpid());

    while (1)
        sleep(1);
}
