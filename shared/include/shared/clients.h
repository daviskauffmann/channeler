#ifndef CLIENT_H
#define CLIENT_H

#include "input.h"
#include "message.h"
#include "player.h"
#include <SDL2/SDL_net.h>

struct client
{
    size_t id;
    TCPsocket socket;
    IPaddress udp_address;
    struct player player;
    struct input input;
};

void clients_init(void);
void clients_reset_input(void);
void clients_broadcast(const void *data, size_t len);

extern struct client clients[MAX_CLIENTS];

#endif
