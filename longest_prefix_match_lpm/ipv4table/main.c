#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_lpm.h>
#include <rte_byteorder.h>
#include <rte_errno.h>
#include <rte_ip.h>


#define MAX_ROUTES 1024
#define MAX_TBL8S 256

int main(int argc, char **argv)
{
    int ret;
    struct rte_lpm *lpm;
    uint32_t next_hop;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    // Configure LPM table parameters
    struct rte_lpm_config config = {
        .max_rules = MAX_ROUTES,      // Maximum number of routing rules
        .number_tbl8s = MAX_TBL8S,    // Number of tbl8 extension tables
        .flags = 0,
    };

    // Create LPM table
    lpm = rte_lpm_create("LPM_TABLE",
                         rte_socket_id(),
                         &config);
    if (lpm == NULL)
        rte_exit(EXIT_FAILURE, "LPM create failed: %s\n",
                 rte_strerror(rte_errno));

    // Add routing entries

    // Route: 10.0.0.0/8 -> next hop 1
    rte_lpm_add(lpm,
                RTE_IPV4(10, 0, 0, 0),
                8,
                1);

    // Route: 10.1.0.0/16 -> next hop 2
    rte_lpm_add(lpm,
                RTE_IPV4(10, 1, 0, 0),
                16,
                2);

    // Route: 10.1.1.0/24 -> next hop 3
    rte_lpm_add(lpm,
                RTE_IPV4(10, 1, 1, 0),
                24,
                3);

    printf("Routes added successfully\n");

    // Destination IP to lookup
    uint32_t dst_ip = RTE_IPV4(10, 1, 1, 25);

    // Perform LPM lookup
    if (rte_lpm_lookup(lpm, dst_ip, &next_hop) == 0) {
        printf("IP %d.%d.%d.%d -> Next hop %u\n",
               (dst_ip >> 24) & 0xFF,
               (dst_ip >> 16) & 0xFF,
               (dst_ip >> 8) & 0xFF,
               dst_ip & 0xFF,
               next_hop);
    } else {
        printf("No route found\n");
    }

    return 0;
}
