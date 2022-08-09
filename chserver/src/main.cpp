#include <SDL2/SDL.h>
#include <ch/map.hpp>
#include <ch/message.hpp>
#include <ch/player.hpp>
#include <ch/server.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <spdlog/spdlog.h>

constexpr std::uint16_t server_port = 8492;

constexpr std::size_t tick_rate = 60;
constexpr std::size_t frame_delay = 1000 / tick_rate;

int main(int, char *[])
{
    if (SDL_Init(SDL_INIT_EVENTS) != 0)
    {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());

        return 1;
    }
    atexit(SDL_Quit);

    if (enet_initialize() != 0)
    {
        spdlog::error("Failed to initialize ENet");

        return 1;
    }
    atexit(enet_deinitialize);

    ch::world world("assets/world.world", "assets/quests.json", "assets/conversations.json");

    ch::server server(server_port, world);
    if (!server.start())
    {
        spdlog::error("Failed to start server");

        return 1;
    }

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

        server.update(delta_time);

        const auto frame_time = SDL_GetTicks() - current_time;
        if (frame_delay > frame_time)
        {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    server.stop();

    return 0;
}
