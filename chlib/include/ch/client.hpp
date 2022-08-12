#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "server.hpp"

struct _ENetHost;
typedef _ENetHost ENetHost;

struct _ENetPeer;
typedef _ENetPeer ENetPeer;

struct _ENetPacket;
typedef _ENetPacket ENetPacket;

namespace ch
{
    struct player;

    class client
    {
    public:
        std::array<ch::connection, ch::server::max_connections> connections;

        client(const char *hostname, std::uint16_t port, ch::world &world);

        bool is_connected() const;

        bool connect();
        bool disconnect();

        void update(float delta_time);

        void send(ENetPacket *packet);

        ch::player &get_player();

    private:
        const char *const hostname;
        const std::uint16_t port;
        ch::world &world;
        ENetHost *host = nullptr;
        ENetPeer *peer = nullptr;
        bool connected = false;
        std::size_t connection_id = ch::server::max_connections;
    };
}

#endif
