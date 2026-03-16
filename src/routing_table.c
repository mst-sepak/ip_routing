#include "routing_table.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

static struct route_entry route_table[] = {
    {0, 0, 0, "ens33"},
    {0, 0, 0, "ens38"}, 
    {0, 0, 0, "ens39"},
};

static const size_t route_table_size = sizeof(route_table) / sizeof(route_table[0]);

// 文字列のIPをuint32_t(network byte order)に変換するソルバ
static uint32_t parse_ipv4(const char *s)
{
    struct in_addr addr;
    if (inet_pton(AF_INET, s, &addr) != 1) {
        fprintf(stderr, "invalid IPv4 address: %s\n", s);
        return 0;
    }
    return addr.s_addr;
}

static void print_route_table(void)
{
    char prefix_str[INET_ADDRSTRLEN];
    char nexthop_str[INET_ADDRSTRLEN];

    printf("Routing table:\n");
    printf("Destination        Prefixlen  NextHop           Iface\n");
    printf("----------------------------------------------------------\n");

    for (size_t i = 0; i < route_table_size; i++) {
        struct in_addr pfx, nh;
        pfx.s_addr = route_table[i].prefix;
        nh.s_addr = route_table[i].next_hop;

        // ネットワークバイトオーダーからホストバイトオーダーへ変換
        if (!inet_ntop(AF_INET, &pfx, prefix_str, sizeof(prefix_str))) {
            strcpy(prefix_str, "invalid");
        }
        if (!inet_ntop(AF_INET, &nh, nexthop_str, sizeof(nexthop_str))) {
            strcpy(nexthop_str, "invalid");
        }

        printf("%-18s  /%-3u      %-16s %s\n",
               prefix_str,
               route_table[i].prefix_len,
               nexthop_str,
               route_table[i].ifname);
    }
}

void init_routing_table(char *cfg) {
    printf("Called init_routing_table\n");

    route_table[0].prefix = parse_ipv4("0.0.0.0");
    route_table[0].prefix_len = 0;
    route_table[0].next_hop = parse_ipv4("192.168.139.2");

    route_table[1].prefix = parse_ipv4("192.168.0.0");
    route_table[1].prefix_len = 24;
    route_table[1].next_hop = parse_ipv4("192.168.233.1");

    route_table[2].prefix = parse_ipv4("10.0.0.0");
    route_table[2].prefix_len = 16;
    route_table[2].next_hop = parse_ipv4("192.168.79.1");

    print_route_table();
}