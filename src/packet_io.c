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
#include <sys/socket.h>
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

struct tx_sock_list {
    struct tx_sock sock[MAX_IF];
    uint8_t num_used;
};

static int init_rx_socket() {
    int rx_fd;

    // 受信用RAWソケット作成
    // AF_INETだとIPより上のプロトコルを指定しないと受信できないので
    // AF_PACKETにしてETH_P_IPを指定することで全IPパケットを受信可能
    rx_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (rx_fd < 0) perror("rx socket");
    
    printf("recv fd: %d\n", rx_fd);
    
    return rx_fd;
}

static struct tx_sock_list *init_tx_socket() {
    
    struct ifaddrs *ifaddr, *ifa;
    struct tx_sock_list *tx_socks = malloc(sizeof(struct tx_sock_list));
    if (!tx_socks) return NULL;
    tx_socks->num_used = 0;
    printf("tx_socks->num_used: %d\n", tx_socks->num_used);
    memset(tx_socks->sock, 0, sizeof(tx_socks->sock));
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
            snprintf(names[if_count], sizeof(names[if_count]), "%s", ifa->ifa_name);
            indexes[if_count] = if_nametoindex(ifa->ifa_name);
            if_count++;
        }
    }

    if (if_count == 0) {
        freeifaddrs(ifaddr);
        return NULL;
    }

    // 実際のソケット作成
    
    for (size_t i = 0; i < if_count; i++) {   
        tx_socks->sock[i].fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (tx_socks->sock[i].fd < 0) {
            perror("socket");
            freeifaddrs(ifaddr);
            return NULL;
        }
        snprintf(tx_socks->sock[i].ifname, IFNAMSIZ, "%s", names[i]);
        tx_socks->sock[i].ifindex = indexes[i];

        tx_socks->num_used++;

        // ソケットへのインターフェースの指定
        if(setsockopt(tx_socks->sock[i].fd, SOL_SOCKET, SO_BINDTODEVICE, tx_socks->sock[i].ifname, strlen(tx_socks->sock[i].ifname)+1) < 0) {
            perror("setsockpt(SO_BINDTODEVICE)");            
        }

        printf("send fd: %d, ifname: %s, ifindex: %d, num_used: %d\n", tx_socks->sock[i].fd, tx_socks->sock[i].ifname, tx_socks->sock[i].ifindex, tx_socks->num_used);
    }    

    return tx_socks;
}

void packet_io_init(int *rx_sock, struct tx_sock_list **tx_sock) {

    *rx_sock = init_rx_socket();
    *tx_sock = init_tx_socket();

}

ssize_t packet_io_recv(int recv_fd, uint8_t *buf, size_t len) {
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

    // lo宛てのパケットははじく
    int lo_ifindex = if_nametoindex("lo");
    if (sll.sll_ifindex == lo_ifindex) return 0;

    return n;
}

int packet_io_send(struct tx_sock_list *tx_sock, const uint8_t *buf, size_t len, struct pkt_meta *meta) {

    struct tx_sock *sock_entry = NULL;

    // metaに入っているインターフェース名にバインドされているソケットを探す
    for (int i = 0; i < tx_sock->num_used; i++) {
        sock_entry = &tx_sock->sock[i];
        if (strcmp(sock_entry->ifname, meta->ifname) == 0) {
            break;
        }
    }
    printf("Output socket is %d\n", sock_entry->fd);

    // L2フレームからイーサヘッダだけ除いたものを送りたい
    const uint8_t *ip_pkt = buf + sizeof(struct ethhdr);
    size_t ip_len = len - sizeof(struct ethhdr);

    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = meta->next_hop;

    ssize_t ret = sendto(sock_entry->fd, ip_pkt, ip_len, 0, (struct sockaddr *)&dst, sizeof(dst));

    if (ret == -1) {
        perror("sendto");
    }
    else if ((size_t)ret != ip_len) {
        printf("一部だけ遅れました\n");
    }

    return 1;
}