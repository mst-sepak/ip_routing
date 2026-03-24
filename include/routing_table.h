#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

/* ルーティングテーブルとLPM検索 */

#include "packet_io.h"

#include <stdint.h>
#include <stddef.h>

#define ROUTE_MAX_ENTRIES 4096

struct route_entry {
    uint32_t prefix;
    uint8_t prefix_len;
    uint32_t next_hop;
    char ifname[16];
};

struct route_table {
    struct route_entry entries[ROUTE_MAX_ENTRIES];
    size_t num_used;    // 今入っているエントリー数
};

struct route_table *route_table_create(void);

void init_route_table(struct route_table *rt, char *cfg);

void add_route(struct route_table *rt, uint32_t prefix, uint8_t prefix_len, uint32_t next_hop, const char* ifname);

int route_table_lookup(struct route_table *rt, uint32_t dst, struct route_entry *out);

void route_table_handle_packet(struct route_table *rt, uint8_t *buf, size_t len, struct pkt_meta *meta);

#endif