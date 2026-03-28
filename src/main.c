#include "routing_table.h"
#include "packet_io.h"
#include "ip_forward.h"
#include "util.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <arpa/inet.h>


int main(void) {
    uint8_t buf[2048];
    struct pkt_meta meta;
    ssize_t n;

    // ローカルIFのIPアドレスリストの作成
    struct local_ip_list *local_ip = local_ip_list_create();
    init_local_ip_list(local_ip);

    // ルーティングテーブルの作成
    struct route_table *rt = route_table_create();
    init_route_table(rt, "routes.conf");

    // ソケットの作成
    int rx_sock;
    struct tx_sock_list *tx_sock = NULL;
    packet_io_init(&rx_sock, &tx_sock);
    if(!tx_sock) return 1;

    int is_local = 0;
    for (;;) {
        
        // 受信用ソケットを使ってパケットを受信
        n = packet_io_recv(rx_sock, buf, sizeof(buf));
        if (n <= 0) continue;
        
        // パケットがローカルIF宛かを確認
        is_local = ip_forward_handle_packet(local_ip, buf);
        if (is_local) continue;

        // パケットの宛先IPがルーティングテーブルに存在するのか確認
        route_table_handle_packet(rt, buf, &meta);
        
        char next_hop_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &meta.next_hop, next_hop_str, sizeof(next_hop_str));
        printf("Next hop is %s\n", next_hop_str);
        printf("Output interface is %s\n", meta.ifname);

        // ルーティングテーブルで指定されているIFからの送信用ソケットを使ってパケットを送信（転送）
        packet_io_send(tx_sock, buf, (size_t)n, &meta);
    }
    
    return 0;
}
