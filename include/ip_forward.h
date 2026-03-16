#ifndef IP_FORWARD_H
#define IP_FORWARD_H

/* 宛先判定、自分宛/転送の分岐 */

#include "packet_io.h"

#include <unistd.h>
#include <stdint.h>

void ip_forward_handle_packet(uint8_t *buf, size_t len, struct pkt_meta *meta);

#endif