#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#include <rte_eal.h>
#include <rte_acl.h>
#include <rte_ip.h>

#define MAX_RULES 10
#define MAX_CATEGORIES 1

// Field indexes
enum {
    PROTO_FIELD,
    SRC_FIELD,
    DST_FIELD,
    SRCP_FIELD,
    DSTP_FIELD,
    NUM_FIELDS
};

// ACL rule structure
struct acl_rule {
    struct rte_acl_rule_data data;
    struct rte_acl_field field[NUM_FIELDS];
};

// ACL field definitions
static struct rte_acl_field_def field_defs[NUM_FIELDS] = {
    [PROTO_FIELD] = {
        .type = RTE_ACL_FIELD_TYPE_BITMASK,
        .size = sizeof(uint8_t),
        .field_index = PROTO_FIELD,
        .input_index = 0,
        .offset = offsetof(struct rte_ipv4_hdr, next_proto_id),
    },
    [SRC_FIELD] = {
        .type = RTE_ACL_FIELD_TYPE_MASK,
        .size = sizeof(uint32_t),
        .field_index = SRC_FIELD,
        .input_index = 1,
        .offset = offsetof(struct rte_ipv4_hdr, src_addr),
    },
    [DST_FIELD] = {
        .type = RTE_ACL_FIELD_TYPE_MASK,
        .size = sizeof(uint32_t),
        .field_index = DST_FIELD,
        .input_index = 2,
        .offset = offsetof(struct rte_ipv4_hdr, dst_addr),
    },
    [SRCP_FIELD] = {
        .type = RTE_ACL_FIELD_TYPE_RANGE,
        .size = sizeof(uint16_t),
        .field_index = SRCP_FIELD,
        .input_index = 3,
        .offset = sizeof(struct rte_ipv4_hdr) + 0,
    },
    [DSTP_FIELD] = {
        .type = RTE_ACL_FIELD_TYPE_RANGE,
        .size = sizeof(uint16_t),
        .field_index = DSTP_FIELD,
        .input_index = 4,
        .offset = sizeof(struct rte_ipv4_hdr) + 2,
    },
};

// Helper function to fill one ACL rule
static void
add_rule(struct acl_rule *rule,
         uint8_t proto,
         uint32_t src, uint32_t src_mask,
         uint32_t dst, uint32_t dst_mask,
         uint16_t sport_low, uint16_t sport_high,
         uint16_t dport_low, uint16_t dport_high,
         uint32_t priority,
         uint32_t userdata)
{
    printf("called add_rule\n");
    memset(rule, 0, sizeof(*rule));

    rule->data.category_mask = 1;
    rule->data.priority = priority;
    rule->data.userdata = userdata;

    rule->field[PROTO_FIELD].value.u8 = proto;
    rule->field[PROTO_FIELD].mask_range.u8 = 0xFF;

    rule->field[SRC_FIELD].value.u32 = src;
    rule->field[SRC_FIELD].mask_range.u32 = src_mask;

    rule->field[DST_FIELD].value.u32 = dst;
    rule->field[DST_FIELD].mask_range.u32 = dst_mask;

    rule->field[SRCP_FIELD].value.u16 = sport_low;
    rule->field[SRCP_FIELD].mask_range.u16 = sport_high;

    rule->field[DSTP_FIELD].value.u16 = dport_low;
    rule->field[DSTP_FIELD].mask_range.u16 = dport_high;
}

int main(int argc, char **argv)
{
    struct rte_acl_param acl_param;
    struct rte_acl_config acl_config;
    struct rte_acl_ctx *ctx;
    struct acl_rule rules[3];

    // Initialize EAL
    rte_eal_init(argc, argv);

    // Create ACL context
    memset(&acl_param, 0, sizeof(acl_param));
    acl_param.name = "ACL_CTX";
    acl_param.socket_id = rte_socket_id();
    acl_param.rule_size = RTE_ACL_RULE_SZ(NUM_FIELDS);
    acl_param.max_rule_num = MAX_RULES;

    ctx = rte_acl_create(&acl_param);
    if (ctx == NULL) {
        printf("ACL create failed\n");
        return -1;
    }

    // Rule 1: Allow DNS UDP port 53
    add_rule(&rules[0],
        IPPROTO_UDP,
        0, 0,
        0, 0,
        0, 65535,
        53, 53,
        100,
        1);

    // Rule 2: Allow HTTP TCP port 80
    add_rule(&rules[1],
        IPPROTO_TCP,
        0, 0,
        0, 0,
        0, 65535,
        80, 80,
        90,
        2);

    // Rule 3: Default catch-all rule
    add_rule(&rules[2],
        0,
        0, 0,
        0, 0,
        0, 65535,
        0, 65535,
        1,
        0);

    // Add rules into ACL
    if (rte_acl_add_rules(ctx, (struct rte_acl_rule *)rules, 3) < 0) {
        printf("Failed to add ACL rules\n");
        return -1;
    }

    // Build ACL trie
    memset(&acl_config, 0, sizeof(acl_config));
    acl_config.num_categories = MAX_CATEGORIES;
    acl_config.num_fields = NUM_FIELDS;
    memcpy(acl_config.defs, field_defs, sizeof(field_defs));

    if (rte_acl_build(ctx, &acl_config) != 0) {
        printf("ACL build failed\n");
        return -1;
    }

    printf("Multi-rule ACL created successfully\n");
    return 0;
}