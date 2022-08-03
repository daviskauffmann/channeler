#include <shared/clients.h>

#include <shared/net.h>

struct client clients[MAX_CLIENTS];

void clients_init(void)
{
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = CLIENT_ID_UNUSED;
    }
}

void clients_reset_input(void)
{
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].id != CLIENT_ID_UNUSED)
        {
            clients[i].input.dx = 0;
            clients[i].input.dy = 0;
        }
    }
}

void clients_broadcast(const void *data, size_t len)
{
    for (size_t i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].id != CLIENT_ID_UNUSED)
        {
            tcp_send(clients[i].socket, data, len);
        }
    }
}
