/*
 * DPDK Control vs Data Packet Segregation
 *
 * Receives packets from a port, classifies each as control-plane or data-plane,
 * and segregates them into separate rings.
 *
 * Without physical NIC (use af_packet on a Linux interface, e.g. veth or tap):
 *   ./main -l 0 --vdev=net_af_packet0,iface=<interface>
 * With physical NIC:
 *   ./main -l 0 -a <PCI_ADDR>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <rte_cycles.h>

#define BURST_SIZE      32
#define RING_SIZE       2048
#define MBUF_POOL_SIZE  8192
#define MBUF_CACHE_SIZE 256

static struct rte_mempool *mbuf_pool;
static struct rte_ring *control_ring;
static struct rte_ring *data_ring;

/* Control-plane: SSH, DNS, BGP, LDAP (TCP); DNS, DHCP, NTP, SNMP (UDP) */

static inline int is_control_tcp_port(uint16_t dport_be) {
    uint16_t port = rte_be_to_cpu_16(dport_be);
    switch (port) {
        case 22: case 53: case 179: case 389:
            return 1;
        default:
            return 0;
    }
}

static inline int is_control_udp_port(uint16_t dport_be) {
    uint16_t port = rte_be_to_cpu_16(dport_be);
    switch (port) {
        case 53: case 67: case 68: case 123: case 161:
            return 1;
        default:
            return 0;
    }
}

/*
 * Classify packet as control (1) or data (0) based on L3/L4.
 * Assumes Ethernet + optional VLAN, then IPv4.
 */
static int classify_packet(struct rte_mbuf *m) {
    uint8_t *pkt = rte_pktmbuf_mtod(m, uint8_t *);
    uint16_t offset = 0;
    uint16_t eth_type;

    if (rte_pktmbuf_data_len(m) < sizeof(struct rte_ether_hdr))
        return 0; /* treat short as data / drop path */

    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)pkt;
    eth_type = rte_be_to_cpu_16(eth->ether_type);
    offset = sizeof(struct rte_ether_hdr);

    if (eth_type == RTE_ETHER_TYPE_VLAN) {
        if (rte_pktmbuf_data_len(m) < offset + sizeof(struct rte_vlan_hdr))
            return 0;
        struct rte_vlan_hdr *vlan = (struct rte_vlan_hdr *)(pkt + offset);
        eth_type = rte_be_to_cpu_16(vlan->eth_proto);
        offset += sizeof(struct rte_vlan_hdr);
    }

    if (eth_type != RTE_ETHER_TYPE_IPV4) {
        /* Non-IP: treat as data (or could treat ARP etc. as control) */
        return 0;
    }

    if (rte_pktmbuf_data_len(m) < offset + sizeof(struct rte_ipv4_hdr))
        return 0;

    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(pkt + offset);
    uint8_t proto = ip->next_proto_id;
    uint16_t ihl = (ip->version_ihl & 0x0F) * 4;
    offset += ihl;

    /* ICMP = control */
    if (proto == IPPROTO_ICMP)
        return 1;

    /* OSPF = control */
    if (proto == 89)
        return 1;

    if (proto == IPPROTO_UDP) {
        if (rte_pktmbuf_data_len(m) < offset + sizeof(struct rte_udp_hdr))
            return 0;
        struct rte_udp_hdr *udp = (struct rte_udp_hdr *)(pkt + offset);
        return is_control_udp_port(udp->dst_port);
    }

    if (proto == IPPROTO_TCP) {
        if (rte_pktmbuf_data_len(m) < offset + sizeof(struct rte_tcp_hdr))
            return 0;
        struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(pkt + offset);
        return is_control_tcp_port(tcp->dst_port);
    }

    return 0;
}

static void init_port(uint16_t port_id) {
    struct rte_eth_conf port_conf = {0};

    if (rte_eth_dev_configure(port_id, 1, 1, &port_conf) < 0)
        rte_exit(EXIT_FAILURE, "Port configure failed\n");

    if (rte_eth_rx_queue_setup(port_id, 0, 1024,
                              rte_eth_dev_socket_id(port_id), NULL, mbuf_pool) < 0)
        rte_exit(EXIT_FAILURE, "RX queue setup failed\n");

    if (rte_eth_tx_queue_setup(port_id, 0, 1024,
                              rte_eth_dev_socket_id(port_id), NULL) < 0)
        rte_exit(EXIT_FAILURE, "TX queue setup failed\n");

    if (rte_eth_dev_start(port_id) < 0)
        rte_exit(EXIT_FAILURE, "Port start failed\n");

    printf("Port %u initialized.\n", port_id);
}

int main(int argc, char **argv) {
    uint16_t port_id = 0;
    uint64_t control_count = 0, data_count = 0;
    uint64_t last_ts = 0;

    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    if (rte_eth_dev_count_avail() == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports found. Use -a <PCI> or --vdev.\n");

    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", MBUF_POOL_SIZE,
                                        MBUF_CACHE_SIZE, 0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE,
                                        rte_socket_id());
    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Cannot create mempool\n");

    control_ring = rte_ring_create("control_ring", RING_SIZE, rte_socket_id(),
                                  RING_F_SP_ENQ | RING_F_SC_DEQ);
    data_ring = rte_ring_create("data_ring", RING_SIZE, rte_socket_id(),
                               RING_F_SP_ENQ | RING_F_SC_DEQ);
    if (!control_ring || !data_ring)
        rte_exit(EXIT_FAILURE, "Cannot create rings\n");

    init_port(port_id);

    printf("Control/Data segregation running. Segregating into rings.\n");
    printf("Control: ICMP, OSPF, DNS/DHCP/NTP/SNMP (UDP), SSH/BGP/LDAP (TCP).\n");
    printf("Data: all other traffic.\n\n");

    while (1) {
        struct rte_mbuf *bufs[BURST_SIZE];
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);

        for (uint16_t i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = bufs[i];
            if (classify_packet(m)) {
                if (rte_ring_enqueue(control_ring, m) == 0)
                    control_count++;
                else
                    rte_pktmbuf_free(m);
            } else {
                if (rte_ring_enqueue(data_ring, m) == 0)
                    data_count++;
                else
                    rte_pktmbuf_free(m);
            }
        }

        /* Drain control ring: re-tx on same port (optional; can free instead) */
        void *out[BURST_SIZE];
        uint16_t n = rte_ring_dequeue_burst(control_ring, out, BURST_SIZE, NULL);
        if (n > 0)
            rte_eth_tx_burst(port_id, 0, (struct rte_mbuf **)out, n);

        n = rte_ring_dequeue_burst(data_ring, out, BURST_SIZE, NULL);
        if (n > 0)
            rte_eth_tx_burst(port_id, 0, (struct rte_mbuf **)out, n);

        /* Simple stats every ~2s */
        uint64_t now = rte_rdtsc();
        if (last_ts == 0) last_ts = now;
        if (now - last_ts > 2e9) {
            printf("Control packets: %" PRIu64 ", Data packets: %" PRIu64 "\n",
                   control_count, data_count);
            last_ts = now;
        }
    }

    return 0;
}
