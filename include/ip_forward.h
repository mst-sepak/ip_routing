#ifndef IP_FORWARD_H
#define IP_FORWARD_H

#include "packet_io.h"

#include <unistd.h>
#include <stdint.h>

int ip_forward_handle_packet(uint8_t *buf, size_t len, struct pkt_meta *meta);

#endif