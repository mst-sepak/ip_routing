#ifndef IP_FORWARD_H
#define IP_FORWARD_H

/* 宛先判定、自分宛/転送の分岐 */

#include "packet_io.h"

#include <unistd.h>
#include <stdint.h>

struct local_ip_list {
    struct in_addr *addrs;
    size_t count;
};

extern struct local_ip_list g_local_ipv4;

int init_local_ipaddr(struct local_ip_list *list);

void ip_forward_handle_packet(uint8_t *buf, size_t len, struct pkt_meta *meta);

#endif