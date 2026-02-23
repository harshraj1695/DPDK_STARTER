#include <stdio.h>
#include <stdint.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_graph.h>
#include <rte_graph_worker.h>

#define BURST 4

static volatile int force_quit;
static int count;

// start1 generates objects and enqueues to work1
static uint16_t
start1_process(struct rte_graph *graph,
               struct rte_node *node,
               void **objs,
               uint16_t nb_objs)
{
    void *out[BURST];

    for (int i = 0; i < BURST; i++)
        out[i] = (void *)(uintptr_t)i;

    printf("START1 -> enqueue %d objs\n", BURST);

    rte_node_enqueue(graph, node, 0, out, BURST);
    return BURST;
}

static struct rte_node_register start1_node = {
    .name = "start1",
    .flags = RTE_NODE_SOURCE_F,
    .process = start1_process,
    .nb_edges = 1,
    .next_nodes = { "work1" },
};

// worker1 receives objects
static uint16_t
work1_process(struct rte_graph *graph,
              struct rte_node *node,
              void **objs,
              uint16_t nb_objs)
{
    printf("WORK1  -> dequeued %u objs\n", nb_objs);
    return nb_objs;
}

static struct rte_node_register work1_node = {
    .name = "work1",
    .process = work1_process,
    .nb_edges = 0,
};

// start2 generates objects and enqueues to work2
static uint16_t
start2_process(struct rte_graph *graph,
               struct rte_node *node,
               void **objs,
               uint16_t nb_objs)
{
    void *out[BURST];

    for (int i = 0; i < BURST; i++)
        out[i] = (void *)(uintptr_t)(100 + i);

    printf("START2 -> enqueue %d objs\n", BURST);

    rte_node_enqueue(graph, node, 0, out, BURST);
    return BURST;
}

static struct rte_node_register start2_node = {
    .name = "start2",
    .flags = RTE_NODE_SOURCE_F,
    .process = start2_process,
    .nb_edges = 1,
    .next_nodes = { "work2" },
};

// worker2 receives objects
static uint16_t
work2_process(struct rte_graph *graph,
              struct rte_node *node,
              void **objs,
              uint16_t nb_objs)
{
    printf("WORK2  -> dequeued %u objs\n", nb_objs);
    return nb_objs;
}

static struct rte_node_register work2_node = {
    .name = "work2",
    .process = work2_process,
    .nb_edges = 0,
};

// register nodes
RTE_NODE_REGISTER(start1_node);
RTE_NODE_REGISTER(work1_node);
RTE_NODE_REGISTER(start2_node);
RTE_NODE_REGISTER(work2_node);

// signal handler to stop loop
static void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
        force_quit = 1;
}

// main uses your graph creation method
int main(int argc, char **argv)
{
    rte_graph_t graph_id;
    struct rte_graph *graph;

    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // graph must include both start nodes
    const char *patterns[] = { "start1", "start2" };

    struct rte_graph_param gparam = {
        .nb_node_patterns = 2,
        .node_patterns = patterns,
    };

    graph_id = rte_graph_create("simple_graph", &gparam);
    if (graph_id == RTE_GRAPH_ID_INVALID)
        rte_exit(EXIT_FAILURE, "Graph creation failed\n");

    graph = rte_graph_lookup("simple_graph");
    if (!graph)
        rte_exit(EXIT_FAILURE, "Graph lookup failed\n");

    printf("Running 2-start-node graph (Ctrl+C to exit)\n");

    while (!force_quit && count < 10) {
        rte_graph_walk(graph);
        count++;
    }

    rte_graph_destroy(graph_id);
    return 0;
}