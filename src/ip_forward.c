#include "ip_forward.h"
#include "routing_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_IF 32

struct local_ip_list g_local_ipv4;

struct local_ip_list {
    struct in_addr addrs[MAX_IF];
    size_t num_used;
};

struct local_ip_list *local_ip_list_create(void)
{
    struct local_ip_list *local_ip = malloc(sizeof(struct local_ip_list));
    if(!local_ip) return NULL;

    local_ip->num_used = 0;
    memset(local_ip->addrs, 0, sizeof(local_ip->addrs));

    return local_ip;
}

int init_local_ip_list(struct local_ip_list *list)
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    // IPv4アドレスを持つIFのアドレスのみをlistに追加
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        
        if (!ifa->ifa_addr) continue;

        // loをスキップ
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        // DOWNはスキップ
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        // アドレスがIPv4であればアドレスをlistに追加
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            list->addrs[list->num_used] = sa->sin_addr;
            list->num_used++;
            
            // デバッグ用
            char ip_addrs[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sa->sin_addr, ip_addrs, sizeof(ip_addrs));
            printf("local (up, non-lo) addr: %s (%s)\n", ip_addrs, ifa->ifa_name);

            // struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            // uint32_t mask = netmask->sin_addr.s_addr;

            // プレフィックス長計算
            // uint32_t mask_host = ntohl(mask);
            // uint8_t prefix_len = 0;
            // while (mask_host & 0x80000000u) {
            //     prefix_len++;
            //     mask_host <<= 1;
            // }
            // uint32_t prefix = sa->sin_addr.s_addr & mask;


        }        
    }

    freeifaddrs(ifaddr);
    return 0;
}

int is_local_dst(struct local_ip_list *list, struct in_addr dst)
{
    for (size_t i = 0; i < list->num_used; i++) {
        if (list->addrs[i].s_addr == dst.s_addr) {
            return 1;
        }
    }
    return 0;
}

// ローカルなら1, ローカルでないなら0
int ip_forward_handle_packet(struct local_ip_list *list, uint8_t *buf) {
    
    struct iphdr *iph = (struct iphdr *)(buf + sizeof(struct ethhdr));
    struct in_addr dst_addr;
    dst_addr.s_addr = iph->daddr;

    char dst_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dst_addr, dst_str, sizeof(dst_str));

    if (is_local_dst(list, dst_addr)) {
        printf("dst %s -> local interface\n", dst_str);
        return 1;
    } else {
        printf("dst %s -> not local\n", dst_str);
        return 0;
    }
}