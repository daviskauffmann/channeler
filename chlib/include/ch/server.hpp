#ifndef CH_SERVER_HPP
#define CH_SERVER_HPP

#include "input.hpp"
#include "player.hpp"
#include <array>
#include <thread>

struct _ENetHost;
typedef _ENetHost ENetHost;

namespace ch
{
    class world;

    struct connection
    {
        std::size_t id;
        ch::player player;
        ch::input input;
    };

    class server
    {
    public:
        static constexpr std::size_t max_connections = 32;

        std::array<ch::connection, max_connections> connections;

        server(std::uint16_t port, ch::world &world);

        bool is_listening() const;

        bool start();
        bool stop();

        void update(float delta_time);

    private:
        const std::uint16_t port;
        ch::world &world;
        ENetHost *host = nullptr;
        bool listening = false;
        std::thread listen_thread;

        void listen();
    };
}

#endif
