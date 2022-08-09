#include <ch/server.hpp>

#include <ch/conversation.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <iostream>

ch::server::server(const std::uint16_t port, ch::world &world)
    : world(world)
{
    connections.fill(
        {.id = max_connections});

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    host = enet_host_create(&address, max_connections, 2, 0, 0);

    std::cout << "[Server] Listening on port " << port << std::endl;
    listen_thread = std::thread(&ch::server::listen, this);
}

ch::server::~server()
{
    listening = false;
    listen_thread.join();

    enet_host_destroy(host);
}

std::size_t ch::server::get_free_connection_id() const
{
    for (std::size_t i = 0; i < max_connections; i++)
    {
        if (connections.at(i).id == max_connections)
        {
            return i;
        }
    }

    return max_connections;
}

void ch::server::update(const float delta_time)
{
    for (std::size_t i = 0; i < world.maps.size(); i++)
    {
        world.maps.at(i).update(delta_time);
    }

    for (auto &connection : connections)
    {
        if (connection.id != max_connections)
        {
            connection.player.update(connection.input, world.maps.at(connection.player.map_index), delta_time);
        }
    }

    {
        ch::message_game_state message;
        message.type = ch::message_type::GAME_STATE;
        for (std::size_t i = 0; i < connections.size(); i++)
        {
            message.connections.at(i).id = connections.at(i).id;

            message.connections.at(i).player.map_index = connections.at(i).player.map_index;

            message.connections.at(i).player.pos_x = connections.at(i).player.pos_x;
            message.connections.at(i).player.pos_y = connections.at(i).player.pos_y;

            message.connections.at(i).player.direction = connections.at(i).player.direction;
            message.connections.at(i).player.animation = connections.at(i).player.animation;
            message.connections.at(i).player.frame_index = connections.at(i).player.frame_index;

            if (connections.at(i).player.conversation_node)
            {
                message.connections.at(i).player.in_conversation = true;
                message.connections.at(i).player.conversation_root_index = connections.at(i).player.conversation_node->root_index;
                message.connections.at(i).player.conversation_node_index = connections.at(i).player.conversation_node->node_index;
            }
            else
            {
                message.connections.at(i).player.in_conversation = false;
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
                std::cout << "[Server] Player connected " << event.peer->address.host << ":" << event.peer->address.port << std::endl;

                const auto new_connection_id = get_free_connection_id();
                if (new_connection_id != max_connections)
                {
                    std::cout << "[Server] Assigning ID " << new_connection_id << std::endl;

                    {
                        ch::message_id message;
                        message.type = ch::message_type::SERVER_JOINED;
                        message.id = new_connection_id;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }

                    for (const auto &connection : connections)
                    {
                        for (const auto &quest_status : connection.player.quest_statuses)
                        {
                            ch::message_quest_status message;
                            message.type = ch::message_type::QUEST_STATUS;
                            message.id = connection.id;
                            message.status = quest_status;
                            auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, 0, packet);
                        }
                    }

                    {
                        auto &new_connection = connections.at(new_connection_id);
                        new_connection.id = new_connection_id;
                        new_connection.player.on_quest_status_set = [this, new_connection](const ch::quest_status &status)
                        {
                            std::cout << "[Server] Player " << new_connection.id << " has advanced quest " << status.quest_index << " to stage " << status.stage_index << std::endl;

                            ch::message_quest_status message;
                            message.type = ch::message_type::QUEST_STATUS;
                            message.id = new_connection.id;
                            message.status = status;
                            auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(host, 0, packet);
                        };
                        event.peer->data = &new_connection;
                    }

                    {
                        ch::message_id message;
                        message.type = ch::message_type::PLAYER_JOINED;
                        message.id = new_connection_id;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(host, 0, packet);
                    }
                }
                else
                {
                    ch::message message;
                    message.type = ch::message_type::SERVER_FULL;
                    auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, packet);
                }
            }
            break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                const auto connection = static_cast<ch::connection *>(event.peer->data);
                const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

                switch (type)
                {
                case ch::message_type::INPUT:
                {
                    const auto message = reinterpret_cast<ch::message_input *>(event.packet->data);

                    connection->input.dx = message->input.dx;
                    connection->input.dy = message->input.dy;
                }
                break;
                case ch::message_type::ATTACK:
                {
                    std::cout << "[Server] Player " << connection->id << " attacking" << std::endl;

                    connection->player.attack();
                }
                break;
                case ch::message_type::CHANGE_MAP:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    std::cout << "[Server] Player " << connection->id << " changing map to " << message->id << std::endl;

                    connection->player.map_index = message->id;
                }
                break;
                case ch::message_type::START_CONVERSATION:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    std::cout << "[Server] Player " << connection->id << " starting conversation " << message->id << std::endl;

                    connection->player.start_conversation(world, message->id);
                }
                break;
                case ch::message_type::ADVANCE_CONVERSATION:
                {
                    std::cout << "[Server] Player " << connection->id << " advancing conversation" << std::endl;

                    connection->player.advance_conversation();
                }
                break;
                case ch::message_type::CHOOSE_CONVERSATION_RESPONSE:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    std::cout << "[Server] Player " << connection->id << " choosing conversation response " << message->id << std::endl;

                    connection->player.choose_conversation_response(message->id);
                }
                break;
                case ch::message_type::END_CONVERSATION:
                {
                    std::cout << "[Server] Player " << connection->id << " ending conversation" << std::endl;

                    connection->player.end_conversation();
                }
                break;
                case ch::message_type::QUEST_STATUS:
                {
                    const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                    std::cout << "[Server] Player " << connection->id << " requesting to change quest " << message->status.quest_index << " to stage " << message->status.stage_index << std::endl;

                    connection->player.set_quest_status(message->status);
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
                const auto connection = static_cast<ch::connection *>(event.peer->data);

                std::cout << "[Server] Player " << connection->id << " disconnected" << std::endl;

                {
                    ch::message_id message;
                    message.type = ch::message_type::PLAYER_DISCONNECTED;
                    message.id = connection->id;
                    auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                    enet_host_broadcast(host, 0, packet);
                }

                {
                    connection->id = max_connections;
                    event.peer->data = nullptr;
                }
            }
            break;
            }
        }
    }
}
