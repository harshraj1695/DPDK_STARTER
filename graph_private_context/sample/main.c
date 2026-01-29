#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_graph.h>
#include <rte_graph_worker.h>
#include <rte_node.h>

/* ---------------- Node names ---------------- */
#define HELLO_NODE   "hello_node"
#define FEATURE_NODE "feature_node"
#define SINK_NODE    "sink_node"

/* ---------------- Feature node private context ---------------- */
struct feature_ctx {
    uint8_t enabled;
};

/* ---------------- HELLO NODE (SOURCE) ---------------- */
static uint16_t
hello_node_process(struct rte_graph *g,
                   struct rte_node *n,
                   void **objs,
                   uint16_t nb_objs)
{
    printf("[hello_node] generating work\n");

    /* Source node must enqueue something */
    rte_node_enqueue(g, n, 0, objs, nb_objs);
    return nb_objs;
}

static struct rte_node_register hello_node = {
    .name = HELLO_NODE,
    .process = hello_node_process,
    .flags = RTE_NODE_SOURCE_F,
    .nb_edges = 1,
    .next_nodes = {
        [0] = FEATURE_NODE,
    },
};
RTE_NODE_REGISTER(hello_node);

/* ---------------- FEATURE NODE ---------------- */
static uint16_t
feature_node_process(struct rte_graph *g,
                     struct rte_node *n,
                     void **objs,
                     uint16_t nb_objs)
{
    struct feature_ctx *ctx = rte_node_ctx_get(n);

    if (ctx->enabled) {
        printf("[feature_node] ENABLED → processing\n");
    } else {
        printf("[feature_node] DISABLED → bypass\n");
    }

    rte_node_enqueue(g, n, 0, objs, nb_objs);
    return nb_objs;
}

static struct rte_node_register feature_node = {
    .name = FEATURE_NODE,
    .process = feature_node_process,
    .nb_edges = 1,
    .next_nodes = {
        [0] = SINK_NODE,
    },
    .size = sizeof(struct feature_ctx),   /* 👈 private context */
};
RTE_NODE_REGISTER(feature_node);

/* ---------------- SINK NODE ---------------- */
static uint16_t
sink_node_process(struct rte_graph *g,
                  struct rte_node *n,
                  void **objs,
                  uint16_t nb_objs)
{
    printf("[sink_node] reached end of graph\n\n");
    return nb_objs;
}

static struct rte_node_register sink_node = {
    .name = SINK_NODE,
    .process = sink_node_process,
    .nb_edges = 0,
};
RTE_NODE_REGISTER(sink_node);

/* ---------------- MAIN ---------------- */
int main(int argc, char **argv)
{
    struct rte_graph *graph;
    rte_graph_t graph_id;
    rte_node_id_t id;
    struct feature_ctx *ctx;

    /* Init EAL */
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    argc -= ret;
    argv += ret;

    if (argc < 2)
        rte_exit(EXIT_FAILURE,
                 "Usage: ./app <0|1> [EAL args]\n");

    /* Create graph */
    struct rte_graph_param gp = {
        .nb_node_patterns = 1,
        .node_patterns = (const char *[]){ HELLO_NODE },
    };

    graph_id = rte_graph_create("ctx_graph", &gp);
    if (graph_id == RTE_GRAPH_ID_INVALID)
        rte_exit(EXIT_FAILURE, "Graph create failed\n");

    graph = rte_graph_lookup("ctx_graph");
    if (!graph)
        rte_exit(EXIT_FAILURE, "Graph lookup failed\n");

    /* Access feature node private context */
    id = rte_node_from_name(FEATURE_NODE);
    ctx = rte_node_ctx_get_by_id(id);

    /* Initialize context */
    ctx->enabled = (uint8_t)atoi(argv[1]);

    printf("Feature initial state = %u\n\n", ctx->enabled);

    /* Dummy object */
    void *objs[1] = { (void *)0xdeadbeef };

    /* Run graph a few times */
    for (int i = 0; i < 5; i++) {
        rte_graph_walk(graph, objs, 1);
    }

    rte_graph_destroy(graph_id);
    rte_eal_cleanup();
    return 0;
}
