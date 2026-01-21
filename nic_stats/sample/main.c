#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_launch.h>
#include <rte_ring.h>
#include <stdio.h>

#include "pipeline.h"

volatile int force_quit = 0;

// Function to print Ethernet statistics
static void print_eth_stats(uint16_t port_id) {
  struct rte_eth_stats stats;
  memset(&stats, 0, sizeof(stats));

  if (rte_eth_stats_get(port_id, &stats) == 0) {
    printf("\n=== rte_eth_stats (port %u) ===\n", port_id);
    printf("RX packets: %" PRIu64 "\n", stats.ipackets);
    printf("TX packets: %" PRIu64 "\n", stats.opackets);
    printf("RX bytes  : %" PRIu64 "\n", stats.ibytes);
    printf("TX bytes  : %" PRIu64 "\n", stats.obytes);

    printf("RX dropped (by HW/PMD): %" PRIu64 "\n", stats.imissed);
    printf("RX errors             : %" PRIu64 "\n", stats.ierrors);
    printf("TX errors             : %" PRIu64 "\n", stats.oerrors);
  } else {
    printf("Failed to get stats for port %u\n", port_id);
  }
}

static void print_xstats(uint16_t port_id) {
  int len = rte_eth_xstats_get(port_id, NULL, 0);
  if (len < 0) {
    printf("Cannot get xstats count\n");
    return;
  }

  struct rte_eth_xstat xstats[len];
  struct rte_eth_xstat_name names[len];

  if (rte_eth_xstats_get_names(port_id, names, len) < 0) {
    printf("Cannot get xstats names\n");
    return;
  }

  if (rte_eth_xstats_get(port_id, xstats, len) < 0) {
    printf("Cannot get xstats\n");
    return;
  }

  printf("\n=== rte_eth_xstats (port %u) ===\n", port_id);
  for (int i = 0; i < len; i++) {
    printf("%s: %" PRIu64 "\n", names[i].name, xstats[i].value);
  }
}

uint64_t rx_count = 0;
uint64_t parser_count = 0;
uint64_t tx_count = 0;

//  Make port_id GLOBAL (fix segfault)
uint16_t port_id = 0;

// Global rings
struct rte_ring *ring_rx_to_parse;
struct rte_ring *ring_parse_to_tx;

int main(int argc, char **argv) {
  rte_eal_init(argc, argv);
  int t = rte_eth_dev_is_valid_port(port_id);
  if (t < 0) {
    rte_exit(EXIT_FAILURE, "Invalid port id %u\n", port_id);
  }

  //  Configure Ethernet port
  struct rte_eth_dev_info dev_info;

  int rt = rte_eth_dev_info_get(port_id, &dev_info);
  if (rt < 0) {
    rte_exit(EXIT_FAILURE, "Error during getting device (port %u) info: %s\n",
             port_id, rte_strerror(-rt));
  }
  printf("Driver name: %s\n", dev_info.driver_name);
  printf("Max RX queues: %u\n", dev_info.max_rx_queues);
  printf("Max TX queues: %u\n", dev_info.max_tx_queues);
  printf("Min RX buffer size: %u\n", dev_info.min_rx_bufsize);
  printf("Max RX packet length: %u\n", dev_info.max_rx_pktlen);

  struct rte_eth_conf port_conf = {0};

  if (rte_eth_dev_configure(port_id, 1, 1, &port_conf) < 0)
    rte_exit(EXIT_FAILURE, "Port configure failed\n");

  struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
      "MBUF_POOL", 8192, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  if (!mbuf_pool)
    rte_exit(EXIT_FAILURE, "MBUF pool create failed\n");

  if (rte_eth_rx_queue_setup(port_id, 0, 1024, rte_socket_id(), NULL,
                             mbuf_pool) < 0)
    rte_exit(EXIT_FAILURE, "RX queue setup failed\n");

  if (rte_eth_tx_queue_setup(port_id, 0, 1024, rte_socket_id(), NULL) < 0)
    rte_exit(EXIT_FAILURE, "TX queue setup failed\n");

  if (rte_eth_dev_start(port_id) < 0)
    rte_exit(EXIT_FAILURE, "Port start failed\n");

  //  Create rings with MP/MC flags
  ring_rx_to_parse =
      rte_ring_create("RING_RX_PARSE", RX_RING_SIZE, rte_socket_id(),
                      RING_F_SP_ENQ | RING_F_SC_DEQ);

  ring_parse_to_tx =
      rte_ring_create("RING_PARSE_TX", TX_RING_SIZE, rte_socket_id(),
                      RING_F_SP_ENQ | RING_F_SC_DEQ);

  if (!ring_rx_to_parse || !ring_parse_to_tx)
    rte_exit(EXIT_FAILURE, "Ring creation failed\n");

  printf("Launching 3 pipeline threads...\n");

  rte_eal_remote_launch(rx_loop, NULL, 1);
  rte_eal_remote_launch(parser_loop, NULL, 2);
  rte_eal_remote_launch(tx_loop, NULL, 3);

  rte_eal_mp_wait_lcore();

  printf("==== PIPELINE RESULTS ====\n");
  printf("RX total = %lu\n", rx_count);
  printf("Parser total = %lu\n", parser_count);
  printf("TX total = %lu\n", tx_count);

  print_eth_stats(port_id);
  print_xstats(port_id);
  return 0;
}
