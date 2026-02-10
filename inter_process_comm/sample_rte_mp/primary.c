#include <rte_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_eal.h>
// #include <rte_mp.h>
int out=0;
// Handler function that gets called when message is received
static int mp_message_handler(const struct rte_mp_msg *msg, const void *peer) {
  int value;

  // Copy payload data from message param buffer
  printf("Size of param: %d\n", msg->len_param);
//   memcpy(&value, msg->param, sizeof(int));
  for(int i=0;i<msg->len_param;i++){
    printf("Primary received param[%d] = %d\n", i, msg->param[i]);
  }
    // printf("Primary received value = %d\n", value);
  out=1;
  return 0;
}

int main(int argc, char **argv) {

  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "EAL init failed\n");

  printf("Running PRIMARY process\n");

  // Register action name with handler function
  // Whenever message with name "sample_mp" arrives, this handler is called
  if (rte_mp_action_register("sample_mp", mp_message_handler) < 0) {
    printf("Handler registration failed\n");
    return -1;
  }

  // Keep primary alive so it can receive messages
  while (!out) {
    printf("Primary waiting for messages...\n");

    sleep(1);
  }

  return 0;
}
