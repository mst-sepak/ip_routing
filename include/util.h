#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

struct pkt_meta
{
    uint32_t next_hop;
    char ifname[16];
};

#endif