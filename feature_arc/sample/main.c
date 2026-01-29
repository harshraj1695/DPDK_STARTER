#include <stdio.h>
#include <string.h>

#define MAX_FEATURES 8
#define MAX_NODES    16

struct node {
    const char *name;
    const char *next;
};

static struct node nodes[MAX_NODES];
static int node_count = 0;

struct feature_arc {
    const char *start;
    const char *end;

    const char *features[MAX_FEATURES];
    int enabled[MAX_FEATURES];
    int count;
};

static struct feature_arc l3_arc;

void add_node(const char *name)
{
    nodes[node_count].name = name;
    nodes[node_count].next = NULL;
    node_count++;
}

void set_next(const char *from, const char *to)
{
    for (int i = 0; i < node_count; i++) {
        if (strcmp(nodes[i].name, from) == 0) {
            nodes[i].next = to;
            return;
        }
    }
}

void feature_arc_init(const char *start, const char *end)
{
    l3_arc.start = start;
    l3_arc.end   = end;
    l3_arc.count = 0;
}

void feature_register(const char *name)
{
    l3_arc.features[l3_arc.count] = name;
    l3_arc.enabled[l3_arc.count]  = 0;
    l3_arc.count++;
}

void feature_enable(const char *name)
{
    for (int i = 0; i < l3_arc.count; i++)
        if (strcmp(l3_arc.features[i], name) == 0)
            l3_arc.enabled[i] = 1;
}

void feature_disable(const char *name)
{
    for (int i = 0; i < l3_arc.count; i++)
        if (strcmp(l3_arc.features[i], name) == 0)
            l3_arc.enabled[i] = 0;
}

void feature_arc_rebuild(void)
{
    const char *prev = l3_arc.start;

    for (int i = 0; i < l3_arc.count; i++) {
        if (l3_arc.enabled[i]) {
            set_next(prev, l3_arc.features[i]);
            prev = l3_arc.features[i];
        }
    }

    set_next(prev, l3_arc.end);
}


void dump_pipeline(void)
{
    printf("Pipeline: ");
    const char *cur = l3_arc.start;

    while (cur) {
        printf("%s", cur);

        const char *next = NULL;
        for (int i = 0; i < node_count; i++)
            if (strcmp(nodes[i].name, cur) == 0)
                next = nodes[i].next;

        if (!next)
            break;

        printf(" → ");
        cur = next;
    }
    printf("\n\n");
}

int main(void)
{
    add_node("L3");
    add_node("Firewall");
    add_node("NAT");
    add_node("QoS");
    add_node("L4");

    feature_arc_init("L3", "L4");
    feature_register("Firewall");
    feature_register("NAT");
    feature_register("QoS");

    // Case 1: nothing enabled
    feature_arc_rebuild();
    dump_pipeline();

    // Case 2: Firewall only
    feature_enable("Firewall");
    feature_arc_rebuild();
    dump_pipeline();

    // Case 3: Firewall + NAT
    feature_enable("NAT");
    feature_arc_rebuild();
    dump_pipeline();

    // Case 4: Only QoS
    feature_disable("Firewall");
    feature_disable("NAT");
    feature_enable("QoS");
    feature_arc_rebuild();
    dump_pipeline();

    // Case 5: All enabled
    feature_enable("Firewall");
    feature_enable("NAT");
    feature_arc_rebuild();
    dump_pipeline();

    return 0;
}
