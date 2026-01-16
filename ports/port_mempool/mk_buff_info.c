#include <stdio.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>

/* Decode helper */
static const char* l2_to_str(uint8_t t)
{
    switch (t) {
        case 1: return "Ethernet";
        case 2: return "VLAN";
        case 3: return "QinQ";
        default: return "Unknown-L2";
    }
}


static const char* l3_to_str(uint8_t t)
{
    switch (t) {
        case 1: return "IPv4";
        case 2: return "IPv4-EXT";
        case 3: return "IPv6";
        case 4: return "IPv6-EXT";
        default: return "Unknown-L3";
    }
}


static const char* l4_to_str(uint8_t t)
{
    switch (t) {
        case 1: return "TCP";
        case 2: return "UDP";
        case 3: return "SCTP";
        case 4: return "ICMP";
        default: return "Unknown-L4";
    }
}


void print_mbuf_info(struct rte_mbuf *m)
{
    printf("\n========== MBUF INFO ==========\n");

    printf("Port: %u\n", m->port);
    printf("Total Packet Length: %u bytes\n", m->pkt_len);
    printf("Segment Data Length: %u bytes\n", m->data_len);
    printf("Segments: %u\n", m->nb_segs);

    printf("RSS Hash: 0x%x\n", m->hash.rss);
    printf("VLAN TCI: %u\n", m->vlan_tci);

    printf("Packet Type Raw: 0x%x\n", m->packet_type);

    printf("L2: %s (%u)\n", l2_to_str(m->l2_type), m->l2_type);
    printf("L3: %s (%u)\n", l3_to_str(m->l3_type), m->l3_type);
    printf("L4: %s (%u)\n", l4_to_str(m->l4_type), m->l4_type);

    printf("Tunnel Type: %u\n", m->tun_type);

    printf("Inner L2: %u | Inner L3: %u | Inner L4: %u\n",
        m->inner_l2_type, m->inner_l3_type, m->inner_l4_type);

    printf("================================\n\n");
}
