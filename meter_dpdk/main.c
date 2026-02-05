#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_flow.h>
#include <rte_mtr.h>

/* =======================
 * Basic RX configuration
 * ======================= */
#define RX_RING_SIZE 1024
#define NUM_MBUFS    8192
#define MBUF_CACHE   250
#define BURST_SIZE   32

static const struct rte_eth_conf port_conf = {
    .rxmode = {
        .max_lro_pkt_size = RTE_ETHER_MAX_LEN,
    },
};

int main(int argc, char **argv)
{
    int ret;
    uint16_t port_id = 0;
    struct rte_mempool *mbuf_pool;

    /* ============================================================
     * 1️⃣ Initialize EAL
     * ============================================================ */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    /* ============================================================
     * 2️⃣ Create mbuf pool (RX buffers)
     * ============================================================ */
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NUM_MBUFS,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "mbuf pool create failed\n");

    /* ============================================================
     * 3️⃣ Configure and start Ethernet port
     * ============================================================ */
    ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "port configure failed\n");

    ret = rte_eth_rx_queue_setup(
        port_id,
        0,
        RX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        NULL,
        mbuf_pool
    );
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "RX queue setup failed\n");

         ret = rte_eth_tx_queue_setup(
        port_id,
        0,
        RX_RING_SIZE,
        rte_eth_dev_socket_id(port_id),
        NULL
    );
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "RX queue setup failed\n");

    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "port start failed\n");

    printf("Port %u started\n", port_id);

    /* ============================================================
     * 4️⃣ CREATE METER PROFILE (rate logic only)
     * ============================================================ */

    struct rte_mtr_meter_profile profile;
    struct rte_mtr_error mtr_err;

    memset(&profile, 0, sizeof(profile));

    /*
     * Algorithm:
     * RTE_MTR_TRTCM_RFC2698 = Two Rate Three Color Marker
     */
    profile.alg = RTE_MTR_TRTCM_RFC2698;

    /*
     * Rates are in BYTES PER SECOND
     */
    profile.trtcm_rfc2698.cir = 10 * 1000 * 1000;  /* 10 Mbps guaranteed */
    profile.trtcm_rfc2698.pir = 20 * 1000 * 1000;  /* 20 Mbps max */
    profile.trtcm_rfc2698.cbs = 64 * 1024;         /* burst @ CIR */
    profile.trtcm_rfc2698.pbs = 128 * 1024;        /* burst @ PIR */

    uint32_t profile_id = 1;

    ret = rte_mtr_meter_profile_add(
        port_id,
        profile_id,
        &profile,
        &mtr_err
    );
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "profile add failed: %s\n",
                 mtr_err.message);

    printf("Meter profile created\n");

    /* ============================================================
     * 5️⃣ CREATE METER POLICY (what to do per color)
     * ============================================================ */

    struct rte_mtr_meter_policy_params policy;
    memset(&policy, 0, sizeof(policy));

    /*
     * For each color, we define an rte_flow_action list
     */

    /* GREEN → pass */
   static struct rte_flow_action green_actions[] = {
    { .type = RTE_FLOW_ACTION_TYPE_END }
};

/* YELLOW → PASS (no action) */
static struct rte_flow_action yellow_actions[] = {
    { .type = RTE_FLOW_ACTION_TYPE_END }
};

/* RED → DROP */
static struct rte_flow_action red_actions[] = {
    { .type = RTE_FLOW_ACTION_TYPE_DROP },
    { .type = RTE_FLOW_ACTION_TYPE_END }
};

    policy.actions[RTE_COLOR_GREEN]  = green_actions;
    policy.actions[RTE_COLOR_YELLOW] = yellow_actions;
    policy.actions[RTE_COLOR_RED]    = red_actions;

    uint32_t policy_id = 1;

    ret = rte_mtr_meter_policy_add(
        port_id,
        policy_id,
        &policy,
        &mtr_err
    );
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "policy add failed: %s\n",
                 mtr_err.message);

    printf("Meter policy created\n");

    /* ============================================================
     * 6️⃣ CREATE METER INSTANCE (bind profile + policy)
     * ============================================================ */

    struct rte_mtr_params params;
    memset(&params, 0, sizeof(params));

    params.meter_profile_id = profile_id;
    params.meter_policy_id  = policy_id;

    uint32_t mtr_id = 1;

    ret = rte_mtr_create(
    port_id,
    mtr_id,
    &params,
    1,              /* shared = 1 → shared meter */
    &mtr_err
);

    if (ret < 0)
        rte_exit(EXIT_FAILURE, "meter create failed: %s\n",
                 mtr_err.message);

    printf("Meter instance created\n");

    /* ============================================================
     * 7️⃣ CREATE FLOW AND ATTACH METER
     * ============================================================ */

    struct rte_flow_attr attr;
    struct rte_flow_item pattern[3];
    struct rte_flow_action actions[2];
    struct rte_flow_error flow_err;

    memset(&attr, 0, sizeof(attr));
    attr.ingress = 1; /* apply on RX */

    memset(pattern, 0, sizeof(pattern));
    pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
    pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
    pattern[2].type = RTE_FLOW_ITEM_TYPE_END;

    memset(actions, 0, sizeof(actions));
    actions[0].type = RTE_FLOW_ACTION_TYPE_METER;
    actions[0].conf = &mtr_id;
    actions[1].type = RTE_FLOW_ACTION_TYPE_END;

    struct rte_flow *flow = rte_flow_create(
        port_id,
        &attr,
        pattern,
        actions,
        &flow_err
    );
    if (!flow)
        rte_exit(EXIT_FAILURE, "flow create failed: %s\n",
                 flow_err.message);

    printf("Flow created and meter attached\n");

    /* ============================================================
     * 8️⃣ RX LOOP
     * ============================================================ */

    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(
            port_id,
            0,
            bufs,
            BURST_SIZE
        );

        if (nb_rx == 0)
            continue;

        for (uint16_t i = 0; i < nb_rx; i++) {
            /*
             * Only GREEN and YELLOW packets arrive here.
             * RED packets are dropped by the policer
             * BEFORE reaching RX queue.
             */
            rte_pktmbuf_free(bufs[i]);
        }
    }

    return 0;
}