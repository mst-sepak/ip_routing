#include "ip_forward.h"

#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_IF 32

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
            list->addrs[list->count++] = sa->sin_addr;

            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
            printf("local (up, non-lo) addr: %s (%s)\n", buf, ifa->ifa_name);           
        }        
    }

    freeifaddrs(ifaddr);
    return 0;
}


void ip_forward_handle_packet(uint8_t *buf, size_t len, struct pkt_meta *meta) {
    
    printf("Called ip_forward_handle_packet\n");

}