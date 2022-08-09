#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "server.hpp"
#include <enet/enet.h>

namespace ch
{
    struct input;
    class player;

    class client
    {
    public:
        std::array<ch::connection, ch::server::max_connections> connections;
        std::size_t connection_id;
        bool listening = true;

        client(const char *hostname, std::uint16_t port, ch::world &world);

        bool connect();
        bool disconnect();

        void send(ENetPacket *packet);

        ch::player &get_player();

    private:
        const char *hostname;
        std::uint16_t port;
        ch::world &world;
        ENetHost *host;
        ENetPeer *peer;
        std::thread listen_thread;

        void listen();
    };
}

#endif
