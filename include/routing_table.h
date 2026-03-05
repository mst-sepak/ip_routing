#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include <stdint.h>

struct route {
    uint32_t prefix;
    uint8_t prefix_len;
    uint32_t next_hop;
    int oif;
};

int routing_table_init(char *cfg);

int routing_table_lookup(uint32_t dst, struct route *out);

#endif