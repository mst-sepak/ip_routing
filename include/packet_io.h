#ifndef PACKET_IO_H
#define PACKET_IO_H

#include <linux/if.h>
#include <unistd.h>
#include <stdint.h>

struct pkt_meta
{
    char ifname[IFNAMSIZ];
    int ifindex;
    uint64_t timestamp_ns;
};


int packet_io_init(void);

ssize_t packet_io_recv(uint8_t *buf, size_t len, struct pkt_meta *m);



#endif