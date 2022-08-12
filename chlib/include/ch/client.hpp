#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "server.hpp"
#include <memory>

struct _ENetPacket;
typedef _ENetPacket ENetPacket;

namespace ch
{
    class host;
    struct input;
    class peer;
    struct player;

    class client
    {
    public:
        std::array<ch::connection, ch::server::max_connections> connections;
        std::size_t connection_id = ch::server::max_connections;

        client(const char *hostname, std::uint16_t port, ch::world &world);
        ~client();

        void update();

        void send(ENetPacket *packet);

        ch::player &get_player();

    private:
        ch::world &world;
        std::unique_ptr<ch::host> host;
        std::unique_ptr<ch::peer> peer;
    };
}

#endif
