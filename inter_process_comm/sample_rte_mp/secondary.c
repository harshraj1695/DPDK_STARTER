#include <rte_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_eal.h>
// #include <rte_mp.h>

// Function to create and send MP message
static void send_message(void)
{
    struct rte_mp_msg msg;

    // Clear structure before using
    memset(&msg, 0, sizeof(msg));

    // Message topic / action name
    // Must match action registered in primary
    strcpy(msg.name, "sample_mp");

    int value = 42;

    // Specify payload size
    msg.len_param = sizeof(int);

    // Copy payload into message buffer
    memcpy(msg.param, &value, sizeof(int));

    printf("Secondary sending message...\n");

    // Send message to all processes connected to MP socket
    if (rte_mp_sendmsg(&msg) < 0) {
        printf("Message send failed\n");
    }
}

int main(int argc, char **argv)
{
    
    // Secondary attaches to primary shared memory and MP socket
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE,"EAL init failed\n");

    printf("Running SECONDARY process\n");

    // Small delay to ensure primary is ready
    sleep(2);

    // Send message to primary
    send_message();

    return 0;
}
