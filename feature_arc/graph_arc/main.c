#include <rte_graph_worker_common.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>

#include <rte_eal.h>
#include <rte_graph.h>
#include <rte_graph_worker.h>

#define HELLO_NODE_NAME "hello_node"
#define DUMMY_NODE_NAME "dummy_node"
#define DUMMY_NODE_NAME2 "dummy_node2"

static volatile int force_quit = 0;
static int count = 0;

// Dynamic global array
static int feature[2] = {0};
// HELLO NODE (source)
// helper function and struct
struct help {
  struct rte_graph *graph;
  struct rte_node *node;
  void **objs;
  uint16_t nb_objs;
};
int helper(void *h) {
  struct help *help_struct = (struct help *)h;
  printf("Helper function called\n");
    printf("Lcore id is %u\n", rte_lcore_id());

  rte_node_enqueue(help_struct->graph, help_struct->node, 1, help_struct->objs,
                   help_struct->nb_objs);
  return 0;
}

static uint16_t hello_node_process(struct rte_graph *graph,
                                   struct rte_node *node, void **objs,
                                   uint16_t nb_objs) {
  printf("Lcore id for source node make  %u\n", rte_lcore_id());

  printf("Hello from graph node!\n");
  count++;
  // Must enqueue SOMETHING to the next node, even empty
  if (feature[0] == 1 && feature[1] == 0) {
    printf("Feature 1 is enabled\n");
    rte_node_enqueue(graph, node, 0, objs, nb_objs);
  } else if (feature[1] == 1 && feature[0] == 0) {
    printf("Feature 2 is enabled\n");
    rte_node_enqueue(graph, node, 1, objs, nb_objs);
  } else if (feature[0] == 1 && feature[1] == 1) {
    printf("Both features are enabled\n");
    rte_node_enqueue(graph, node, 0, objs, nb_objs);
    // rte_node_enqueue(graph, node, 1, objs, nb_objs);
    struct help h;
    h.graph = graph;
    h.node = node;
    h.objs = objs;
    h.nb_objs = nb_objs;
    rte_eal_remote_launch(helper, (void *)&h, 1);

  } else {
    printf("No features enabled exiting the program\n");
    force_quit = 1;
  }
  return 0;
}

static struct rte_node_register hello_node = {
    .name = HELLO_NODE_NAME,
    .process = hello_node_process,
    .flags = RTE_NODE_SOURCE_F,

    .nb_edges = 2, // MUST HAVE AT LEAST ONE EDGE
    .next_nodes =
        {
            [0] = DUMMY_NODE_NAME,  // Dummy next node
            [1] = DUMMY_NODE_NAME2, // second dummy
        },
};
RTE_NODE_REGISTER(hello_node);

// DUMMY NODE (does nothing)
static uint16_t dummy_node_process(struct rte_graph *graph,
                                   struct rte_node *node, void **objs,
                                   uint16_t nb_objs) {
  // Does nothing
  printf("Going into dummy node 1\n");
  printf("Lcore id id %u\n", rte_lcore_id());
  count++;
  return nb_objs;
}

static struct rte_node_register dummy_node = {
    .name = DUMMY_NODE_NAME,
    .process = dummy_node_process,
    .nb_edges = 0,
};
RTE_NODE_REGISTER(dummy_node);

// second dummy NODE
static uint16_t dummy_node_process2(struct rte_graph *graph,
                                    struct rte_node *node, void **objs,
                                    uint16_t nb_objs) {
  // Does nothing
  printf("Going into dummy node 2\n");
  printf("Lcore id id %u\n", rte_lcore_id());

  count++;
  return nb_objs;
}

static struct rte_node_register dummy_node2 = {
    .name = DUMMY_NODE_NAME2,
    .process = dummy_node_process2,
    .nb_edges = 0,
};
RTE_NODE_REGISTER(dummy_node2);

static void signal_handler(int sig) {
  if (sig == SIGINT || sig == SIGTERM)
    force_quit = 1;
}

int main(int argc, char **argv) {
  rte_graph_t graph_id;
  struct rte_graph *graph;

  int ret;

  ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "EAL init failed\n");

  // Remove EAL args from argc/argv

  argc -= ret;
  argv += ret;

  // Now argv contains ONLY your args
  if (argc < 3)
    rte_exit(EXIT_FAILURE, "Usage: ./main <f1> <f2> [EAL args]\n");

  int f1 = (int)strtol(argv[1], NULL, 10);
  int f2 = (int)strtol(argv[2], NULL, 10);

  printf("Runtime flags: f1=%d f2=%d\n", f1, f2);

  feature[0] = f1;
  feature[1] = f2;

  struct rte_graph_param gparam = {
      .nb_node_patterns = 1,
      .node_patterns = (const char *[]){HELLO_NODE_NAME},
  };

  graph_id = rte_graph_create("simple_graph", &gparam);
  if (graph_id == RTE_GRAPH_ID_INVALID)
    rte_exit(EXIT_FAILURE, "Graph creation failed\n");

  graph = rte_graph_lookup("simple_graph");
  if (!graph)
    rte_exit(EXIT_FAILURE, "Graph lookup failed\n");

  printf("Running minimal DPDK graph (Ctrl+C to exit)\n");

  while (!force_quit && count < 1) {
    rte_graph_walk(graph);
  }

  rte_graph_destroy(graph_id);
  return 0;
}
