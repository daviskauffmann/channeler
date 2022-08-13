#include <SDL2/SDL.h>
#include <ch/enet.hpp>
#include <ch/platform.hpp>
#include <ch/server.hpp>
#include <ch/world.hpp>
#include <spdlog/spdlog.h>

constexpr std::uint16_t server_port = 8492;

int main(int, char *[])
{
    ch::platform platform(SDL_INIT_EVENTS);

    const ch::enet enet;

    ch::world world("assets/world.world", "assets/quests.json", "assets/conversations.json");

    ch::server server(server_port, world);

    while (platform.is_running())
    {
        const auto delta_time = platform.get_delta_time();

        SDL_Event event;
        while (platform.poll_event(event))
        {
            server.handle_event(event);
        }

        server.update(delta_time);
    }

    return 0;
}
