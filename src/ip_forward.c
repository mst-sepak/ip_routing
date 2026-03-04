#include "ip_forward.h"

#include <stdio.h>

int ip_forward_handle_packet(uint8_t *buf, size_t len, struct pkt_meta *meta) {
    
    printf("Called ip_forward_handle_packet\n");

    return 1;
}