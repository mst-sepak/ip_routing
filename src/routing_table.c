#include "routing_table.h"
#include "ip_forward.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

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

struct route_table *route_table_create(void)
{
    struct route_table *rt = malloc(sizeof(struct route_table));
    if (!rt) return NULL;

    rt->num_used = 0;
    memset(rt->entries, 0, sizeof(rt->entries));

    return rt;
}

void add_route(struct route_table *rt, uint32_t prefix, uint8_t prefix_len, uint32_t next_hop, const char* ifname)
{
    rt->entries[rt->num_used].prefix        = prefix;
    rt->entries[rt->num_used].prefix_len    = prefix_len;
    rt->entries[rt->num_used].next_hop      = next_hop;
    snprintf(rt->entries[rt->num_used].ifname, sizeof(rt->entries[rt->num_used].ifname) - 1, "%s", ifname);
    rt->entries[rt->num_used].ifname[sizeof(rt->entries[rt->num_used].ifname) - 1] = '\0';
    rt->num_used++;
}

static void add_direct_route(struct route_table *rt)
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        
        if (!ifa->ifa_addr) continue;

        // loをスキップ
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        // DOWNはスキップ
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        // アドレスがIPv4であればアドレスをlistに追加
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;       // IFのIPアドレス
            struct sockaddr_in *mask = (struct sockaddr_in *)ifa->ifa_netmask;  // IFのサブネットマスク

            uint32_t prefix = sa->sin_addr.s_addr & mask->sin_addr.s_addr;      // IPアドレスとサブネットマスクのANDがプレフィックス

            // プレフィックス長を計算（マスクをホストバイトオーダーにしたら先頭から1が並ぶので、その数を数える）
            uint32_t mask_host = ntohl(mask->sin_addr.s_addr);
            uint8_t prefix_len = 0;
            while (mask_host & 0x80000000u) {
                prefix_len++;
                mask_host <<= 1;
            }

            add_route(rt, prefix, prefix_len, 0, ifa->ifa_name);
        }
    }
}

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

static void print_route_table(struct route_table *rt)
{
    char prefix_str[INET_ADDRSTRLEN];
    char nexthop_str[INET_ADDRSTRLEN];

    printf("Routing table:\n");
    printf("Destination        Prefixlen  NextHop           Iface\n");
    printf("----------------------------------------------------------\n");

    for (size_t i = 0; i < rt->num_used; i++) {
        struct in_addr pfx, nh;
        pfx.s_addr = rt->entries[i].prefix;
        nh.s_addr = rt->entries[i].next_hop;

        // ネットワークバイトオーダーからホストバイトオーダーへ変換
        if (!inet_ntop(AF_INET, &pfx, prefix_str, sizeof(prefix_str))) {
            strcpy(prefix_str, "invalid");
        }
        if (!inet_ntop(AF_INET, &nh, nexthop_str, sizeof(nexthop_str))) {
            strcpy(nexthop_str, "invalid");
        }

        printf("%-18s  /%-3u      %-16s %s\n",
               prefix_str,
               rt->entries[i].prefix_len,
               nexthop_str,
               rt->entries[i].ifname);
    }
}

void init_route_table(struct route_table *rt, char *cfg) {
    printf("Called init_routing_table\n");

    add_direct_route(rt);

    uint32_t prefix;
    uint8_t prefix_len;
    uint32_t next_hop;
    char ifname[16];

    prefix        = parse_ipv4("0.0.0.0");
    prefix_len    = 0;
    next_hop      = parse_ipv4("192.168.139.2");
    strcpy(ifname, "ens33");
    add_route(rt, prefix, prefix_len, next_hop, ifname);

    prefix        = parse_ipv4("192.168.0.0");
    prefix_len    = 24;
    next_hop      = parse_ipv4("192.168.233.1");
    strcpy(ifname, "ens38");
    add_route(rt, prefix, prefix_len, next_hop, ifname);
    
    prefix        = parse_ipv4("10.0.0.0");
    prefix_len    = 16;
    next_hop      = parse_ipv4("192.168.79.1");
    strcpy(ifname, "ens39");
    add_route(rt, prefix, prefix_len, next_hop, ifname);
    
    print_route_table(rt);
}

static uint32_t prefixlen_to_mask(uint8_t prefix_len)
{
    if (prefix_len == 0) {
        return 0u;
    }

    // /24や/16などのマスクをビット列に変換（ネットワークバイトオーダー）
    uint32_t mask = htonl(0xFFFFFFFFu << (32 - prefix_len));
    return mask;
}

static int prefix_match(uint32_t dst, const struct route_entry *re)
{
    uint32_t mask = prefixlen_to_mask(re->prefix_len);
    return (dst & mask) == (re->prefix & mask);
}

static const struct route_entry *route_table_lookup(struct route_table *rt, uint32_t dst)
{
    const struct route_entry *best = NULL;
    uint8_t best_plen = 0;

    for (size_t i = 0; i < rt->num_used; i++) {
        const struct route_entry *re = &rt->entries[i];

        if(!prefix_match(dst, re)) {
            continue;
        }

        if (re->prefix_len > best_plen) {
            best = re;
            best_plen = re->prefix_len;
        }
    }

    return best;
}

static void print_route_entry(const struct route_entry *re, uint32_t dst_addr)
{
    char prefix_str[INET_ADDRSTRLEN];
    char nexthop_str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &re->prefix, prefix_str, sizeof(prefix_str));
    if (re->next_hop != 0) {
        inet_ntop(AF_INET, &re->next_hop, nexthop_str, sizeof(nexthop_str));
    }
    else {
        inet_ntop(AF_INET, &dst_addr, nexthop_str, sizeof(nexthop_str));
    }

    printf("prefix=%s/%u nexthop=%s if=%s\n", prefix_str, re->prefix_len, nexthop_str, re->ifname);
}

void route_table_handle_packet(struct route_table *rt, uint8_t *buf, struct pkt_meta *meta)
{

    printf("Called route_table_handle_packet\n");
    
    struct iphdr *iph = (struct iphdr *)(buf + sizeof(struct ethhdr));
    struct in_addr dst_addr;
    dst_addr.s_addr = iph->daddr;
    char dst_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dst_addr, dst_str, sizeof(dst_str));

    const struct route_entry *re = route_table_lookup(rt, dst_addr.s_addr);
    if (!re) {  // エントリが見つからなかった場合
        printf("No route for %s\n", dst_str);
    }
    else {    // エントリが見つかった場合
        print_route_entry(re, dst_addr.s_addr);
        if (re->next_hop != 0) {    // 間接ルートの場合、ルーティングエントリのネクストホップを返す
            meta->next_hop = re->next_hop;
        } 
        else {      // 直接ルートの場合、パケットの宛先アドレスをネクストホップとして返す
            meta->next_hop = dst_addr.s_addr;
        }
        snprintf(meta->ifname, sizeof(meta->ifname), "%s", re->ifname);
    }

}