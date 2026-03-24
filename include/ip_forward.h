#ifndef IP_FORWARD_H
#define IP_FORWARD_H

/* 宛先判定、自分宛/転送の分岐 */

#include "packet_io.h"

#include <unistd.h>
#include <stdint.h>

struct local_ip_list;

extern struct local_ip_list g_local_ipv4;

struct local_ip_list *local_ip_list_create(void);

int init_local_ip_list(struct local_ip_list *list);

int ip_forward_handle_packet(struct local_ip_list *list, uint8_t *buf, size_t len, struct pkt_meta *meta);

#endif