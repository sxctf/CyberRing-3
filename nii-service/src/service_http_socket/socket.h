#ifndef SOCK_H
#define SOCK_H

int rx_open(uint16_t port);
void rx_packet(int sock);

#endif