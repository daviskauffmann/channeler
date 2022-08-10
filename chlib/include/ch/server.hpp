#ifndef CH_SERVER_HPP
#define CH_SERVER_HPP

#include "input.hpp"
#include "player.hpp"
#include <array>
#include <memory>
#include <thread>

namespace ch
{
    class host;
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
        bool listening = true;

        server(std::uint16_t port, ch::world &world);
        ~server();

        std::size_t get_free_connection_id() const;

        void update(float delta_time);

    private:
        ch::world &world;
        std::unique_ptr<ch::host> host;
        std::thread listen_thread;

        void listen();
    };
}

#endif
