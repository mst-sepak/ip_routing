#include "packet_io.h"
#include "ip_forward.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <ifaddrs.h>

#define MAX_IF 32

struct tx_sock {
    int fd;
    char ifname[IFNAMSIZ];
    unsigned int ifindex;
};

static int recv_fd = -1;
static struct tx_sock *send_fd = NULL;

int init_rx_socket() {
    int rx_fd;

    // 受信用RAWソケット作成
    // AF_INETだとIPより上のプロトコルを指定しないと受信できないので
    // AF_PACKETにしてETH_P_IPを指定することで全IPパケットを受信可能
    rx_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (rx_fd < 0) perror("rx socket");
    
    printf("recv fd: %d\n", rx_fd);
    
    return rx_fd;
}

struct tx_sock *init_tx_socket() {
    
    struct ifaddrs *ifaddr, *ifa;
    struct tx_sock *socks = NULL;
    size_t if_count = 0;

    char names[MAX_IF][IFNAMSIZ];
    unsigned int indexes[MAX_IF];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return NULL;
    }

    // 物理IFの数を数える
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;    // loは除外
        if (!(ifa->ifa_flags & IFF_UP)) continue;       // DOWNは除外
        
        // すでに登録済みのIF名ならスキップ
        size_t i;
        for (i = 0; i < if_count; i++) {
            if (strncmp(names[i], ifa->ifa_name, IFNAMSIZ) == 0)
                break;
        }
        // breakでforループを抜けた場合は、i < if_countなので、次のIFへ
        if (i < if_count)
            continue;

        // 現在のIFと過去の全てのIFを比較して被らなかったら新しいIFとして登録
        if (if_count < MAX_IF) {
            strncpy(names[if_count], ifa->ifa_name, IFNAMSIZ);
            indexes[if_count] = if_nametoindex(ifa->ifa_name);
            if_count++;
        }
    }

    if (if_count == 0) {
        freeifaddrs(ifaddr);
        return NULL;
    }

    socks = malloc(sizeof(struct tx_sock) * if_count);
    if (!socks) {
        freeifaddrs(ifaddr);
        return NULL;
    }

    // 実際のソケット作成
    for (size_t i = 0; i < if_count; i++) {   
        struct tx_sock *t = socks;
        t->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (t->fd < 0) {
            perror("socket");
            freeifaddrs(ifaddr);
            return NULL;
        }
        strncpy(t->ifname, names[i], IFNAMSIZ);
        t->ifindex = indexes[i];

        printf("send fd: %d, ifname: %s, ifindex: %d\n", t->fd, t->ifname, t->ifindex);

        socks++;
    }    

    return socks;
}

int packet_io_init(void) {

    recv_fd = init_rx_socket();
    send_fd = init_tx_socket();

    return 0;
}

int packet_io_get_fd(void) {
    return recv_fd;
}

ssize_t packet_io_recv(uint8_t *buf, size_t len, struct pkt_meta *meta) {
    struct sockaddr_ll sll;
    socklen_t slen = sizeof(sll);
    ssize_t n;

    if (recv_fd < 0) {
        fprintf(stderr, "packet_io_recv: raw socket not initialized\n");
        return -1;
    }
    
    n = recvfrom(recv_fd, buf, len, 0, (struct sockaddr *)&sll, &slen);
    if (n < 0) {
        perror("recvfrom");
        return -1;
    }

    // 受信インターフェースとL3プロトコルの確認
    // printf("ifindex = %d\n", sll.sll_ifindex);
    // char if_name[IFNAMSIZ];
    // if_indextoname(sll.sll_ifindex, if_name);
    // printf("ifname = %s\n", if_name);
    // printf("protocol = 0x%04x\n", ntohs(sll.sll_protocol));

    if (meta) {
        meta->ifindex = sll.sll_ifindex;
        if (!if_indextoname(sll.sll_ifindex, meta->ifname)) {
            perror("if_indextoname");
            return -1;
        }
    }

    // lo宛てのパケットははじく
    int lo_ifindex = if_nametoindex("lo");
    if (sll.sll_ifindex == lo_ifindex) return 0;

    ip_forward_handle_packet(buf, n, meta);

    // パケットの宛先IPアドレスを表示
    /*
    struct iphdr *iph = (struct iphdr *)(buf + sizeof(struct ethhdr));
    struct in_addr dst_addr;
    dst_addr.s_addr = iph->daddr;
    // printf("dst ip = %s\n", inet_ntoa(dst_addr));
    */

    return 0;
}

int packet_io_send(const uint8_t *buf, size_t len, const struct route_entry *rt) {
    printf("Called packet_io_send");
    return 1;
}