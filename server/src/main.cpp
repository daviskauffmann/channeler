#include <SDL2/SDL.h>
#include <enet/enet.h>
#include <shared/conversations.hpp>
#include <shared/map.hpp>
#include <shared/player.hpp>
#include <shared/quests.hpp>
#include <shared/world.hpp>

constexpr std::uint16_t server_port = 8492;

constexpr std::size_t tick_rate = 60;
constexpr std::size_t frame_delay = 1000 / tick_rate;

constexpr std::size_t max_clients = 32;

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVENTS) != 0)
    {
        return 1;
    }

    if (enet_initialize() != 0)
    {
        return 1;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = server_port;
    auto server = enet_host_create(&address, max_clients, 2, 0, 0);
    if (!server)
    {
        return 1;
    }

    hp::world world("assets/world.world");
    hp::quests quests("assets/quests.json");
    hp::conversations conversations("assets/conversations.json");

    bool quit = false;
    while (!quit)
    {
        static uint32_t current_time = 0;
        uint32_t frame_start = SDL_GetTicks();
        uint32_t previous_time = current_time;
        current_time = frame_start;
        float delta_time = (current_time - previous_time) / 1000.0f;

        {
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
        }

        {
            ENetEvent event;
            /* Wait up to 1000 milliseconds for an event. */
            while (enet_host_service(server, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("A new client connected from %x:%u.\n",
                           event.peer->address.host,
                           event.peer->address.port);
                    /* Store any relevant client information here. */
                    event.peer->data = (void *)"Client information";
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                           event.packet->dataLength,
                           event.packet->data,
                           event.peer->data,
                           event.channelID);
                    /* Clean up the packet now that we're done using it. */
                    enet_packet_destroy(event.packet);

                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("%s disconnected.\n", event.peer->data);
                    /* Reset the peer's client information. */
                    event.peer->data = NULL;
                }
            }
        }

        uint32_t frame_end = SDL_GetTicks();
        uint32_t frame_time = frame_end - frame_start;
        if (frame_delay > frame_time)
        {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    enet_host_destroy(server);
    enet_deinitialize();

    SDL_Quit();

    return 0;
}
