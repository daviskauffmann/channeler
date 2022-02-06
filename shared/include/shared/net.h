#ifndef NET_H
#define NET_H

#include <SDL2/SDL_net.h>
#include <stdbool.h>

bool tcp_send(TCPsocket sock, const void *data, size_t len);
bool udp_send(UDPsocket sock, IPaddress address, const void *data, size_t len);

#endif
