#ifndef PACKET_IO_H
#define PACKET_IO_H

/* RAWソケットでの受信・送信 */

#include "routing_table.h"

#include <net/if.h>
#include <unistd.h>
#include <stdint.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

struct pkt_meta
{
    char ifname[IFNAMSIZ];
    int ifindex;
    uint64_t timestamp_ns;
};


int packet_io_init(void);

int packet_io_get_fd(void);

ssize_t packet_io_recv(uint8_t *buf, size_t len, struct pkt_meta *meta);

int pakcet_io_send(const uint8_t *buf, size_t len, const struct route_entry *rt);

#endif