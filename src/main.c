#include "packet_io.h"
#include "routing_table.h"
#include "ip_forward.h"

#include <stdint.h>
#include <unistd.h>

int main(void) {
    uint8_t buf[2048];
    struct pkt_meta meta;
    ssize_t n;

    packet_io_init();
    routing_table_init("routes.conf");

    for (;;) {
        n = packet_io_recv(buf, sizeof(buf), &meta);
        if (n <= 0) continue;
        ip_forward_handle_packet(buf, (size_t)n, &meta);
    }
    
    return 0;
}
