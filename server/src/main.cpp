#include <SDL2/SDL.h>
#include <enet/enet.h>
#include <iostream>
#include <shared/client_list.hpp>
#include <shared/conversation_list.hpp>
#include <shared/map.hpp>
#include <shared/message.hpp>
#include <shared/player.hpp>
#include <shared/quest_list.hpp>
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

    hp::client_list client_list;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = server_port;
    auto host = enet_host_create(&address, client_list.max_clients, 2, 0, 0);
    if (!host)
    {
        return 1;
    }

    hp::world world("assets/world.world");
    hp::quest_list quest_list("assets/quest_list.json");
    hp::conversation_list conversation_list("assets/conversation_list.json");

    auto quit = false;
    while (!quit)
    {
        static std::uint32_t current_time = 0;
        const auto previous_time = current_time;
        current_time = SDL_GetTicks();
        const auto delta_time = (current_time - previous_time) / 1000.0f;

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
            while (enet_host_service(host, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    std::cout << "Connect " << event.peer->address.host << ":" << event.peer->address.port << std::endl;

                    const auto new_client_id = client_list.get_available_client();
                    if (new_client_id != client_list.max_clients)
                    {
                        std::cout << "Assigned ID " << new_client_id << std::endl;

                        {
                            hp::message_id message;
                            message.type = hp::message_type::SERVER_JOINED;
                            message.id = new_client_id;
                            auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, 0, packet);
                        }

                        {
                            auto &new_client = client_list.clients.at(new_client_id);
                            new_client.id = new_client_id;
                            event.peer->data = &new_client;
                        }

                        {
                            hp::message_id message;
                            message.type = hp::message_type::CLIENT_JOINED;
                            message.id = new_client_id;
                            auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(host, 0, packet);
                        }
                    }
                    else
                    {
                        hp::message message;
                        message.type = hp::message_type::SERVER_JOINED;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }
                }
                break;
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    const auto client = static_cast<hp::client *>(event.peer->data);
                    const auto type = reinterpret_cast<hp::message *>(event.packet->data)->type;

                    switch (type)
                    {
                    case hp::message_type::INPUT:
                    {
                        const auto message = reinterpret_cast<hp::message_input *>(event.packet->data);

                        client->input.dx = message->input.dx;
                        client->input.dy = message->input.dy;
                    }
                    break;
                    case hp::message_type::CHANGE_MAP:
                    {
                        const auto message = reinterpret_cast<hp::message_id *>(event.packet->data);

                        client->player.map_index = message->id;

                        std::cout << "Change map " << client->id << " " << message->id << std::endl;
                    }
                    break;
                    default:
                    {
                        std::cout << "Unknown message type " << static_cast<int>(type) << std::endl;
                    }
                    break;
                    }

                    enet_packet_destroy(event.packet);
                }
                break;
                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    const auto client = static_cast<hp::client *>(event.peer->data);
                    const auto disconnected_client_id = client->id;

                    std::cout << "Disconnect " << disconnected_client_id << std::endl;

                    {
                        client->id = client_list.max_clients;
                        event.peer->data = nullptr;
                    }

                    {
                        hp::message_id message;
                        message.type = hp::message_type::CLIENT_DISCONNECTED;
                        message.id = disconnected_client_id;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(host, 0, packet);
                    }
                }
                break;
                }
            }
        }

        for (std::size_t i = 0; i < world.maps.size(); i++)
        {
            world.maps.at(i).update(delta_time);
        }

        for (auto &client : client_list.clients)
        {
            if (client.id != client_list.max_clients)
            {
                client.player.update(client.input, world.maps.at(client.player.map_index), delta_time);
            }
        }

        {
            hp::message_game_state message;
            message.type = hp::message_type::GAME_STATE;
            for (std::size_t i = 0; i < client_list.clients.size(); i++)
            {
                message.clients.at(i).id = client_list.clients.at(i).id;

                message.clients.at(i).player.map_index = client_list.clients.at(i).player.map_index;

                message.clients.at(i).player.pos_x = client_list.clients.at(i).player.pos_x;
                message.clients.at(i).player.pos_y = client_list.clients.at(i).player.pos_y;

                message.clients.at(i).player.direction = client_list.clients.at(i).player.direction;
                message.clients.at(i).player.animation = client_list.clients.at(i).player.animation;
                message.clients.at(i).player.frame_index = client_list.clients.at(i).player.frame_index;
            }

            auto packet = enet_packet_create(&message, sizeof(message), 0);
            enet_host_broadcast(host, 0, packet);
        }

        const auto frame_time = SDL_GetTicks() - current_time;
        if (frame_delay > frame_time)
        {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    enet_host_destroy(host);

    return 0;
}
