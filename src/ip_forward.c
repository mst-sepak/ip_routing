#include "ip_forward.h"
#include "routing_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_IF 32

struct local_ip_list g_local_ipv4;

int init_local_ipaddr(struct local_ip_list *list)
{
    struct ifaddrs *ifaddr, *ifa;
    size_t if_count = 0;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    // まず数える
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        // loをスキップ
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        // DOWNはスキップ
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            if_count++;
        }
    }

    list->addrs = calloc(if_count, sizeof(struct in_addr));
    if (!list->addrs) {
        freeifaddrs(ifaddr);
        return -1;
    }

    list->count = 0;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        // loをスキップ
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        // DOWNはスキップ
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            list->addrs[list->count++] = sa->sin_addr;

            uint32_t mask = netmask->sin_addr.s_addr;

            // プレフィックス長計算
            uint32_t mask_host = ntohl(mask);
            uint8_t prefix_len = 0;
            while (mask_host & 0x80000000u) {
                prefix_len++;
                mask_host <<= 1;
            }
            uint32_t prefix = sa->sin_addr.s_addr & mask;

            char ip_addrs[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sa->sin_addr, ip_addrs, sizeof(ip_addrs));
            printf("local (up, non-lo) addr: %s/%d (%s)\n", ip_addrs, prefix_len, ifa->ifa_name);
        }        
    }

    freeifaddrs(ifaddr);
    return 0;
}

int is_local_dst(struct in_addr dst)
{
    for (size_t i = 0; i < g_local_ipv4.count; i++) {
        if (g_local_ipv4.addrs[i].s_addr == dst.s_addr) {
            return 1;
        }
    }
    return 0;
}

void ip_forward_handle_packet(uint8_t *buf, size_t len, struct pkt_meta *meta) {
    
    //printf("Called ip_forward_handle_packet\n");

    struct iphdr *iph = (struct iphdr *)(buf + sizeof(struct ethhdr));
    struct in_addr dst_addr;
    dst_addr.s_addr = iph->daddr;


    char dst_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dst_addr, dst_str, sizeof(dst_str));

    if (is_local_dst(dst_addr)) {
        printf("dst %s -> local interface\n", dst_str);
    } else {
        printf("dst %s -> not local\n", dst_str);
    }
}