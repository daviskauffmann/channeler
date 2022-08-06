#include <SDL2/SDL.h>
#include <enet/enet.h>
#include <iostream>
#include <shared/client_list.hpp>
#include <shared/map.hpp>
#include <shared/message.hpp>
#include <shared/player.hpp>
#include <shared/server.hpp>
#include <shared/world.hpp>

constexpr std::uint16_t server_port = 8492;

constexpr std::size_t tick_rate = 60;
constexpr std::size_t frame_delay = 1000 / tick_rate;

int main(int, char *[])
{
    if (SDL_Init(SDL_INIT_EVENTS) != 0)
    {
        return 1;
    }
    atexit(SDL_Quit);

    if (enet_initialize() != 0)
    {
        return 1;
    }
    atexit(enet_deinitialize);

    hp::server server(server_port);

    hp::world world("assets/world.world", "assets/quests.json", "assets/conversations.json");

    auto quit = false;
    while (!quit)
    {
        static std::uint32_t current_time = 0;
        const auto previous_time = current_time;
        current_time = SDL_GetTicks();
        const auto delta_time = (current_time - previous_time) / 1000.0f;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
            {
                quit = true;
            }
            break;
            }
        }

        server.update(delta_time, world);

        const auto frame_time = SDL_GetTicks() - current_time;
        if (frame_delay > frame_time)
        {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    return 0;
}
