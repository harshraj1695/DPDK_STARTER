#include <stdio.h>
#include <unistd.h>
#include <rte_eal.h>
#include <rte_graph.h>
#include <rte_graph_worker.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#define NB_MBUF 1024

static struct rte_mempool *mbuf_pool;
static volatile int total_prints = 0;

/* ---------------- START NODE 1 ---------------- */

static uint16_t
start2_process(struct rte_graph *graph, struct rte_node *node,
               void **objs, uint16_t nb_objs)
{
    if (total_prints >= 10)
        return 0;

    printf("START2 -> producing packet\n");

    struct rte_mbuf *m = rte_pktmbuf_alloc(mbuf_pool);
    if (!m)
        return 0;

    void *arr[1] = { m };

    rte_node_enqueue(graph, node, 0, arr, 1);

    total_prints++;
    usleep(200000);

    return 0;
}

static struct rte_node_register start2_node = {
    .name = "start2",
    .flags = RTE_NODE_SOURCE_F,
    .process = start2_process,
    .nb_edges = 1,
    .next_nodes = { "worker" },
};
RTE_NODE_REGISTER(start2_node);


static uint16_t
start1_process(struct rte_graph *graph, struct rte_node *node,
               void **objs, uint16_t nb_objs)
{
    if (total_prints >= 10)
        return 0;

    printf("START1 -> producing packet\n");

    struct rte_mbuf *m = rte_pktmbuf_alloc(mbuf_pool);
    if (!m)
        return 0;

    void *arr[1] = { m };

    // push packet to worker node
    rte_node_enqueue(graph, node, 0, arr, 1);

    total_prints++;
    usleep(200000);   // slow down scheduler so logs readable

    return 0;
}

static struct rte_node_register start1_node = {
    .name = "start1",
    .flags = RTE_NODE_SOURCE_F ,
    .process = start1_process,
    .nb_edges = 1,
    .next_nodes = { "worker" },
};
RTE_NODE_REGISTER(start1_node);

/* ---------------- START NODE 2 ---------------- */

// static uint16_t
// start2_process(struct rte_graph *graph, struct rte_node *node,
//                void **objs, uint16_t nb_objs)
// {
//     if (total_prints >= 10)
//         return 0;

//     printf("START2 -> producing packet\n");

//     struct rte_mbuf *m = rte_pktmbuf_alloc(mbuf_pool);
//     if (!m)
//         return 0;

//     void *arr[1] = { m };

//     rte_node_enqueue(graph, node, 0, arr, 1);

//     total_prints++;
//     usleep(200000);

//     return 0;
// }

// static struct rte_node_register start2_node = {
//     .name = "start2",
//     .flags = RTE_NODE_SOURCE_F,
//     .process = start2_process,
//     .nb_edges = 1,
//     .next_nodes = { "worker" },
// };
// RTE_NODE_REGISTER(start2_node);

/* ---------------- WORKER NODE ---------------- */

static uint16_t
worker_process(struct rte_graph *graph, struct rte_node *node,
               void **objs, uint16_t nb_objs)
{
    printf("WORKER <- received %u packet(s)\n", nb_objs);

    for (uint16_t i = 0; i < nb_objs; i++)
        rte_pktmbuf_free(objs[i]);

    return nb_objs;
}

static struct rte_node_register worker_node = {
    .name = "worker",
    .process = worker_process,
    .nb_edges = 0,
};
RTE_NODE_REGISTER(worker_node);

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv)
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                        NB_MBUF,
                                        256, 0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE,
                                        rte_socket_id());
    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    const char *patterns[] = {
        "start1",
        "start2",
        NULL,
    };

    struct rte_graph_param gconf = {
        .socket_id = rte_socket_id(),
        .nb_node_patterns = 2,
        .node_patterns = patterns,
    };

    rte_graph_t id = rte_graph_create("debug_graph", &gconf);
    if (id == RTE_GRAPH_ID_INVALID)
        rte_exit(EXIT_FAILURE, "Graph creation failed\n");

    struct rte_graph *graph = rte_graph_lookup("debug_graph");
    if (!graph)
        rte_exit(EXIT_FAILURE, "Graph lookup failed\n");

    printf("Running graph debug example...\n");

    while (total_prints < 10)
        rte_graph_walk(graph);

    printf("Stopping after 10 prints.\n");
    return 0;
}
