#include "packet_io.h"
#include "routing_table.h"
#include "ip_forward.h"

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

    packet_io_init();
    if (init_local_ipaddr(&g_local_ipv4) < 0) {
        return 1;
    }
    //int fd = packet_io_get_fd();
    //printf("socket fd is %d\n", fd);

    init_routing_table("routes.conf");

    for (;;) {
        n = packet_io_recv(buf, sizeof(buf), &meta);
        //printf("Received packet len: %ld\n", n);
        if (n <= 0) continue;
        ip_forward_handle_packet(buf, (size_t)n, &meta);
    }
    
    return 0;
}
