#ifndef PACKET_IO_H
#define PACKET_IO_H

/* RAWソケットでの受信・送信 */

#include "routing_table.h"
#include "util.h"

#include <net/if.h>
#include <unistd.h>
#include <stdint.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

// struct pkt_meta
// {
//     char ifname[IFNAMSIZ];
//     int ifindex;
//     uint64_t timestamp_ns;
// };

struct tx_sock_list;

void packet_io_init(int *rx_sock, struct tx_sock_list **tx_sock);

ssize_t packet_io_recv(int recv_fd, uint8_t *buf, size_t len);

int packet_io_send(struct tx_sock_list *tx_sock, const uint8_t *buf, size_t len, struct pkt_meta *meta);

#endif