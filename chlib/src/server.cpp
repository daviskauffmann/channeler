#include <ch/server.hpp>

#include <ch/conversation.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <iostream>

ch::server::server(const std::uint16_t port, ch::world &world)
    : world(world)
{
    clients.fill(
        {.id = max_clients});

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    host = enet_host_create(&address, max_clients, 2, 0, 0);

    std::cout << "[Server] Listening on port " << port << std::endl;

    listen_thread = std::thread(&ch::server::listen, this);
}

ch::server::~server()
{
    listening = false;
    listen_thread.join();

    enet_host_destroy(host);
}

std::size_t ch::server::get_available_client() const
{
    for (std::size_t i = 0; i < max_clients; i++)
    {
        if (clients.at(i).id == max_clients)
        {
            return i;
        }
    }

    return max_clients;
}

void ch::server::update(const float delta_time)
{
    for (std::size_t i = 0; i < world.maps.size(); i++)
    {
        world.maps.at(i).update(delta_time);
    }

    for (auto &client : clients)
    {
        if (client.id != max_clients)
        {
            client.player.update(client.input, world.maps.at(client.player.map_index), delta_time);
        }
    }

    {
        ch::message_game_state message;
        message.type = ch::message_type::GAME_STATE;
        for (std::size_t i = 0; i < clients.size(); i++)
        {
            message.clients.at(i).id = clients.at(i).id;

            message.clients.at(i).player.map_index = clients.at(i).player.map_index;

            message.clients.at(i).player.pos_x = clients.at(i).player.pos_x;
            message.clients.at(i).player.pos_y = clients.at(i).player.pos_y;

            message.clients.at(i).player.direction = clients.at(i).player.direction;
            message.clients.at(i).player.animation = clients.at(i).player.animation;
            message.clients.at(i).player.frame_index = clients.at(i).player.frame_index;

            if (clients.at(i).player.conversation_node)
            {
                message.clients.at(i).player.in_conversation = true;
                message.clients.at(i).player.conversation_root_index = clients.at(i).player.conversation_node->root_index;
                message.clients.at(i).player.conversation_node_index = clients.at(i).player.conversation_node->node_index;
            }
            else
            {
                message.clients.at(i).player.in_conversation = false;
            }
        }

        auto packet = enet_packet_create(&message, sizeof(message), 0);
        enet_host_broadcast(host, 0, packet);
    }
}

void ch::server::listen()
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

                const auto new_client_id = get_available_client();
                if (new_client_id != max_clients)
                {
                    std::cout << "[Server] Assigning ID " << new_client_id << std::endl;

                    {
                        ch::message_id message;
                        message.type = ch::message_type::SERVER_JOINED;
                        message.id = new_client_id;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }

                    {
                        auto &new_client = clients.at(new_client_id);
                        new_client.id = new_client_id;
                        event.peer->data = &new_client;
                    }

                    {
                        ch::message_id message;
                        message.type = ch::message_type::CLIENT_JOINED;
                        message.id = new_client_id;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(host, 0, packet);
                    }
                }
                else
                {
                    ch::message message;
                    message.type = ch::message_type::SERVER_JOINED;
                    auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, packet);
                }
            }
            break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                const auto client = static_cast<ch::client *>(event.peer->data);
                const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

                switch (type)
                {
                case ch::message_type::INPUT:
                {
                    const auto message = reinterpret_cast<ch::message_input *>(event.packet->data);

                    client->input.dx = message->input.dx;
                    client->input.dy = message->input.dy;
                }
                break;
                case ch::message_type::ATTACK:
                {
                    client->player.attack();
                }
                break;
                case ch::message_type::CHANGE_MAP:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);
                    const auto map_index = message->id;

                    std::cout << "[Server] Client " << client->id << " changing map to " << map_index << std::endl;

                    client->player.map_index = map_index;
                }
                break;
                case ch::message_type::START_CONVERSATION:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);
                    const auto root_index = message->id;

                    client->player.start_conversation(world, root_index);
                }
                break;
                case ch::message_type::ADVANCE_CONVERSATION:
                {
                    client->player.advance_conversation();
                }
                break;
                case ch::message_type::CHOOSE_CONVERSATION_RESPONSE:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);
                    const auto choice_index = message->id;

                    client->player.choose_conversation_response(choice_index);
                }
                break;
                case ch::message_type::END_CONVERSATION:
                {
                    client->player.end_conversation();
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
                const auto client = static_cast<ch::client *>(event.peer->data);
                const auto disconnected_client_id = client->id;

                std::cout << "[Server] Client " << disconnected_client_id << " disconnected" << std::endl;

                {
                    client->id = max_clients;
                    event.peer->data = nullptr;
                }

                {
                    ch::message_id message;
                    message.type = ch::message_type::CLIENT_DISCONNECTED;
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
