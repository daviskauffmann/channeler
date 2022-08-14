#include <SDL2/SDL.h>
#include <ch/enet.hpp>
#include <ch/sdl.hpp>
#include <ch/server.hpp>
#include <ch/world.hpp>
#include <spdlog/spdlog.h>

constexpr std::uint16_t server_port = 8492;

int main(int, char *[])
{
    ch::sdl sdl(SDL_INIT_EVENTS);

    const ch::enet enet;

    ch::world world("assets/world.world", "assets/quests.json", "assets/conversations.json");

    ch::server server(server_port, world);

    std::uint64_t current_time = 0;
    bool running = true;
    while (running)
    {
        const auto previous_time = current_time;
        current_time = sdl.get_ticks();
        const auto delta_time = (current_time - previous_time) / 1000.0f;

        SDL_Event event;
        while (sdl.poll_event(event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
            {
                running = false;
            }
            break;
            }

            server.handle_event(event);
        }

        server.update(delta_time);
    }

    return 0;
}
