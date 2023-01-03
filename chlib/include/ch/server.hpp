#ifndef CH_SERVER_HPP
#define CH_SERVER_HPP

#include "player.hpp"
#include <SDL2/SDL.h>
#include <array>
#include <memory>
#include <thread>

namespace ch
{
    class host;
    class world;

    class server
    {
    public:
        static constexpr std::size_t max_players = 32;

        std::array<ch::player, max_players> players;

        server(
            std::uint16_t port,
            std::shared_ptr<ch::world> world);
        ~server();
        server(const server &other) = delete;
        server &operator=(const server &other) = delete;
        server(server &&other) = delete;
        server &operator=(server &&other) = delete;

        void handle_event(const SDL_Event &event);
        void update(float delta_time);

    private:
        std::shared_ptr<ch::world> world;
        std::unique_ptr<ch::host> host;
        bool listening;
        std::thread listen_thread;

        void listen();
    };
}

#endif
