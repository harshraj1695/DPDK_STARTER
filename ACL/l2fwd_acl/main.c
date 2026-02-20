/* SPDX-License-Identifier: BSD-3-Clause
 * l2fwd_acl example: demonstrates use of DPDK ACL library
 */

#include <stdbool.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>

#include <rte_acl.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_log.h>

#define NUM_FIELDS 5
#define MAX_RULES  32
#define NB_MBUF    8192
#define BURST      32

/* ---- Correct DPDK logging style ---- */
#define RTE_LOGTYPE_L2FWD_ACL RTE_LOGTYPE_USER1
#define LOGTYPE_L2FWD_ACL RTE_LOGTYPE_L2FWD_ACL

RTE_ACL_RULE_DEF(acl_rule, NUM_FIELDS);

struct pkt_key {
    uint8_t  proto;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
};

enum {
    PROTO_FIELD,
    SRC_FIELD,
    DST_FIELD,
    SPORT_FIELD,
    DPORT_FIELD
};

static volatile bool force_quit;
static uint32_t portmask = 0x1;

static struct rte_acl_field_def field_defs[NUM_FIELDS] = {
    { RTE_ACL_FIELD_TYPE_BITMASK, sizeof(uint8_t),  PROTO_FIELD, 0, offsetof(struct pkt_key, proto) },
    { RTE_ACL_FIELD_TYPE_MASK,    sizeof(uint32_t), SRC_FIELD,   1, offsetof(struct pkt_key, src_ip) },
    { RTE_ACL_FIELD_TYPE_MASK,    sizeof(uint32_t), DST_FIELD,   2, offsetof(struct pkt_key, dst_ip) },
    { RTE_ACL_FIELD_TYPE_RANGE,   sizeof(uint16_t), SPORT_FIELD, 3, offsetof(struct pkt_key, src_port) },
    { RTE_ACL_FIELD_TYPE_RANGE,   sizeof(uint16_t), DPORT_FIELD, 3, offsetof(struct pkt_key, dst_port) },
};

static void
signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
        force_quit = true;
}

static int
parse_args(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            portmask = strtoul(optarg, NULL, 16);
            break;
        default:
            return -1;
        }
    }
    return 0;
}

/* ---------------- ACL INITIALIZATION ---------------- */

static struct rte_acl_ctx *
init_acl(void)
{
    struct rte_acl_param prm = {
        .name = "acl_ctx",
        .socket_id = rte_socket_id(),
        .rule_size = RTE_ACL_RULE_SZ(NUM_FIELDS),
        .max_rule_num = MAX_RULES
    };

    struct rte_acl_ctx *ctx = rte_acl_create(&prm);
    if (!ctx)
        rte_exit(EXIT_FAILURE, "ACL create failed\n");

    struct acl_rule rule;
    memset(&rule, 0, sizeof(rule));

    rule.data.priority = 10;
    rule.data.category_mask = 1;
    rule.data.userdata = 1;

    rule.field[PROTO_FIELD].value.u8 = IPPROTO_TCP;
    rule.field[PROTO_FIELD].mask_range.u8 = 0xFF;

    rule.field[SRC_FIELD].value.u32 =
        rte_be_to_cpu_32(RTE_IPV4(172,17,166,200));
    rule.field[SRC_FIELD].mask_range.u32 = 32;

    rule.field[DST_FIELD].value.u32 = 0;
    rule.field[DST_FIELD].mask_range.u32 = 0;

    rule.field[SPORT_FIELD].value.u16 = 0;
    rule.field[SPORT_FIELD].mask_range.u16 = 65535;

    rule.field[DPORT_FIELD].value.u16 = 0;
    rule.field[DPORT_FIELD].mask_range.u16 = 65535;

    if (rte_acl_add_rules(ctx, (struct rte_acl_rule *)&rule, 1) < 0)
        rte_exit(EXIT_FAILURE, "ACL rule add failed\n");

    struct rte_acl_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.num_categories = 1;
    cfg.num_fields = NUM_FIELDS;
    memcpy(cfg.defs, field_defs, sizeof(field_defs));

    if (rte_acl_build(ctx, &cfg) != 0)
        rte_exit(EXIT_FAILURE, "ACL build failed\n");

    RTE_LOG(INFO, L2FWD_ACL, "ACL ready\n");
    return ctx;
}

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv)
{
    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    argc -= ret;
    argv += ret;

    if (parse_args(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "Invalid arguments\n");

    struct rte_acl_ctx *acl_ctx = init_acl();

    struct rte_mempool *mp = rte_pktmbuf_pool_create(
        "mbuf_pool", NB_MBUF, 256, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (!mp)
        rte_exit(EXIT_FAILURE, "mempool failed\n");

    uint16_t port_id;
    RTE_ETH_FOREACH_DEV(port_id) {

        if (!(portmask & (1 << port_id)))
            continue;

        struct rte_eth_conf conf = {0};

        if (rte_eth_dev_configure(port_id, 1, 1, &conf) < 0)
            rte_exit(EXIT_FAILURE, "port configure failed\n");

        if (rte_eth_rx_queue_setup(port_id, 0, 1024,
            rte_eth_dev_socket_id(port_id), NULL, mp) < 0)
            rte_exit(EXIT_FAILURE, "rx setup failed\n");

        if (rte_eth_tx_queue_setup(port_id, 0, 1024,
            rte_eth_dev_socket_id(port_id), NULL) < 0)
            rte_exit(EXIT_FAILURE, "tx setup failed\n");

        if (rte_eth_dev_start(port_id) < 0)
            rte_exit(EXIT_FAILURE, "port start failed\n");

        rte_eth_promiscuous_enable(port_id);

        RTE_LOG(INFO, L2FWD_ACL, "Started port %u\n", port_id);
    }

    struct rte_mbuf *bufs[BURST];

    while (!force_quit) {

        RTE_ETH_FOREACH_DEV(port_id) {

            if (!(portmask & (1 << port_id)))
                continue;

            uint16_t nb = rte_eth_rx_burst(port_id, 0, bufs, BURST);
            if (!nb) continue;

            for (int i = 0; i < nb; i++) {

                struct rte_mbuf *m = bufs[i];
                struct rte_ether_hdr *eth =
                    rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

                if (eth->ether_type !=
                    rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                    rte_pktmbuf_free(m);
                    continue;
                }

                struct rte_ipv4_hdr *ip =
                    (struct rte_ipv4_hdr *)(eth + 1);

                struct pkt_key key = {0};
                key.proto  = ip->next_proto_id;
                key.src_ip = rte_be_to_cpu_32(ip->src_addr);
                key.dst_ip = rte_be_to_cpu_32(ip->dst_addr);

                uint8_t *l4 = (uint8_t *)ip + (ip->ihl * 4);

                if (key.proto == IPPROTO_TCP) {
                    struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)l4;
                    key.src_port = rte_be_to_cpu_16(tcp->src_port);
                    key.dst_port = rte_be_to_cpu_16(tcp->dst_port);
                } else if (key.proto == IPPROTO_UDP) {
                    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)l4;
                    key.src_port = rte_be_to_cpu_16(udp->src_port);
                    key.dst_port = rte_be_to_cpu_16(udp->dst_port);
                }

                const uint8_t *data[1] = {(const uint8_t *)&key};
                uint32_t res[1];

                rte_acl_classify(acl_ctx, data, res, 1, 1);

                if (res[0])
                    rte_eth_tx_burst(port_id, 0, &m, 1);
                else
                    rte_pktmbuf_free(m);
            }
        }
    }

    return 0;
}
