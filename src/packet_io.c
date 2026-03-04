#include "packet_io.h"

#include <stdio.h>

int packet_io_init(void) {
    printf("Called packet_io_init\n");
    return 1;
}

ssize_t packet_io_recv(uint8_t *buf, size_t len, struct pkt_meta *m) {
    printf("Called packet_io_recv\n");
    return 1;
}