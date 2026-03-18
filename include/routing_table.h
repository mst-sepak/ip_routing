#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

/* ルーティングテーブルとLPM検索 */

#include <stdint.h>

struct route_entry {
    uint32_t prefix;
    uint8_t prefix_len;
    uint32_t next_hop;
    char ifname[16];
};

void init_routing_table(char *cfg);

void add_route(uint32_t prefix, uint8_t prefix_len, 
                uint32_t next_hop, const char* ifname);

int routing_table_lookup(uint32_t dst, struct route_entry *out);

#endif