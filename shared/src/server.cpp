#include <shared/server.hpp>

#include <iostream>
#include <shared/message.hpp>
#include <shared/world.hpp>

hp::server::server(const std::uint16_t port)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    host = enet_host_create(&address, client_list.max_clients, 2, 0, 0);

    std::cout << "[Server] Listening on port " << port << std::endl;

    listen_thread = std::thread(&hp::server::listen, this);
}

hp::server::~server()
{
    listening = false;
    listen_thread.join();

    enet_host_destroy(host);
}

void hp::server::update(const float delta_time, hp::world &world)
{
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
}

void hp::server::listen()
{
    while (listening)
    {
        ENetEvent event;
        while (enet_host_service(host, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                std::cout << "[Server] Client connected " << event.peer->address.host << ":" << event.peer->address.port << std::endl;

                const auto new_client_id = client_list.get_available_client();
                if (new_client_id != client_list.max_clients)
                {
                    std::cout << "[Server] Assigning ID " << new_client_id << std::endl;

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

                    std::cout << "[Server] Client " << client->id << " changing map to " << message->id << std::endl;
                }
                break;
                default:
                {
                    std::cout << "[Server] Unknown message type " << static_cast<int>(type) << std::endl;
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

                std::cout << "[Server] Client " << disconnected_client_id << " disconnected" << std::endl;

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
}
