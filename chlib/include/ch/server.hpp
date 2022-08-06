#ifndef CH_SERVER_HPP
#define CH_SERVER_HPP

#include "input.hpp"
#include "player.hpp"
#include <array>
#include <enet/enet.h>
#include <thread>

namespace ch
{
    class world;

    struct client
    {
        std::size_t id;
        ch::player player;
        ch::input input;
    };

    class server
    {
    public:
        static constexpr std::size_t max_clients = 32;

        std::array<ch::client, max_clients> clients;
        bool listening = true;

        server(std::uint16_t port);
        ~server();

        std::size_t get_available_client() const;

        void update(float delta_time, ch::world &world);

    private:
        ENetHost *host;
        std::thread listen_thread;

        void listen();
    };
}

#endif
