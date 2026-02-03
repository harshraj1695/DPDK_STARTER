#include <rte_common.h>
#include <rte_eal.h>
#include <rte_eventdev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define EVENT_DEV_ID 0
#define NUM_QUEUES 1
#define NUM_PORTS 1

static int event_worker(void *arg) {
  struct rte_event ev;
  uint8_t dev_id = EVENT_DEV_ID;
  uint8_t port_id = 0;

  printf("Worker running on lcore %u\n", rte_lcore_id());

  while (1) {
    if (rte_event_dequeue_burst(dev_id, port_id, &ev, 1, 0)) {
      printf("Worker lcore %u got event, data=%lu, flow_id=%u\n",
             rte_lcore_id(), ev.u64, ev.flow_id);
      break;
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  int rt = rte_eal_init(argc, argv);
  if (rt < 0) {
    rte_exit(EXIT_FAILURE, "Eal init failed\n");
  }
  printf("Eal initialized\n");

  struct rte_event_dev_config dev_conf = {0};
  dev_conf.nb_event_queues = NUM_QUEUES;
  dev_conf.nb_event_ports = NUM_PORTS;
  dev_conf.nb_event_port_dequeue_depth = 32;
  dev_conf.nb_event_port_enqueue_depth = 32;
  dev_conf.nb_event_queue_flows = 1024;
  dev_conf.nb_events_limit = 4096;

  if (rte_event_dev_configure(EVENT_DEV_ID, &dev_conf) < 0) {
    rte_exit(EXIT_FAILURE, "configure failed\n");
  }
  printf("Event dev configured\n");

  struct rte_event_queue_conf qconf = {0};
  qconf.schedule_type = RTE_SCHED_TYPE_ATOMIC;
  qconf.nb_atomic_flows = 1024;
  qconf.nb_atomic_order_sequences = 1024;
  qconf.priority = RTE_EVENT_DEV_PRIORITY_NORMAL;

  if (rte_event_queue_setup(EVENT_DEV_ID, 0, &qconf)) {
    rte_exit(EXIT_FAILURE, "Queue setup failed\n");
  }
  printf("Queue created\n");

  struct rte_event_port_conf pconf;
  rte_event_port_default_conf_get(EVENT_DEV_ID, 0, &pconf);

  if (rte_event_port_setup(EVENT_DEV_ID, 0, &pconf)) {
    rte_exit(EXIT_FAILURE, "Port setup failed\n");
  }
  printf("Port created\n");

  uint8_t queue_ids[NUM_QUEUES] = {0};
  uint8_t priorities[] = {RTE_EVENT_DEV_PRIORITY_NORMAL};

  if (rte_event_port_link(EVENT_DEV_ID, 0, queue_ids, priorities, 1) < 0) {
    rte_exit(EXIT_FAILURE, "Port link failed\n");
  }

  if (rte_event_dev_start(EVENT_DEV_ID) < 0)
    rte_exit(EXIT_FAILURE, "Start failed\n");

  printf("Event device started\n");

  printf("Port linked to queue\n");

  unsigned int lcore_id;

  // Launch worker on worker lcore
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    rte_eal_remote_launch(event_worker, NULL, lcore_id);
    break; // launch only one worker
  }

  // Give worker time to start
  rte_delay_ms(100);

  // Create and enqueue one event
  struct rte_event ev = {0};
  ev.queue_id = 0;
  ev.op = RTE_EVENT_OP_NEW;
  ev.sched_type = RTE_SCHED_TYPE_ATOMIC;
  ev.event_type = RTE_EVENT_TYPE_CPU;
  ev.flow_id = 1;
  ev.u64 = 42;

  printf("Main lcore enqueuing event\n");

  while (rte_event_enqueue_burst(EVENT_DEV_ID, 0, &ev, 1) != 1)
    rte_pause();

  // Wait for worker to finish
  rte_eal_mp_wait_lcore();
  printf("Event processing completed\n");

   return 0;
   
}