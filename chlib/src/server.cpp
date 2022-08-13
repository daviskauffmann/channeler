#include <ch/server.hpp>

#include <algorithm>
#include <ch/conversation.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <spdlog/spdlog.h>

ch::server::server(const std::uint16_t port, ch::world &world)
    : port(port),
      world(world)
{
    connections.fill(
        {.id = max_connections});
}

bool ch::server::is_listening() const
{
    return listening;
}

bool ch::server::start()
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    host = enet_host_create(&address, max_connections, 2, 0, 0);
    if (!host)
    {
        spdlog::error("[Server] Failed to create host");

        return false;
    }

    listening = true;
    listen_thread = std::thread(&ch::server::listen, this);

    spdlog::info("[Server] Started on port {}", port);

    return true;
}

bool ch::server::stop()
{
    listening = false;
    listen_thread.join();
    listen_thread = {};

    enet_host_destroy(host);
    host = nullptr;

    spdlog::info("[Server] Successfully stopped");

    return true;
}

void ch::server::update(const float delta_time)
{
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
                spdlog::info("[Server] Player connected {}:{}", event.peer->address.host, event.peer->address.port);

                const auto new_connection = std::find_if(
                    connections.begin(),
                    connections.end(),
                    [](const ch::connection &connection)
                    {
                        return connection.id == max_connections;
                    });
                if (new_connection != connections.end())
                {
                    new_connection->id = std::distance(connections.begin(), new_connection);
                    new_connection->player.on_quest_status_set = [this, new_connection](const ch::quest_status &status)
                    {
                        spdlog::info("[Server] Player {} has advanced quest {} to stage {}", new_connection->id, status.quest_index, status.stage_index);

                        ch::message_quest_status message;
                        message.type = ch::message_type::QUEST_STATUS;
                        message.id = new_connection->id;
                        message.status = status;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(host, 0, packet);
                    };
                    event.peer->data = &*new_connection;

                    spdlog::info("[Server] Assigned ID {}", new_connection->id);

                    {
                        ch::message_id message;
                        message.type = ch::message_type::SERVER_JOINED;
                        message.id = new_connection->id;
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
                        ch::message_id message;
                        message.type = ch::message_type::PLAYER_JOINED;
                        message.id = new_connection->id;
                        auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(host, 0, packet);
                    }
                }
                else
                {
                    spdlog::warn("[Server] Player tried to join, but server is full");

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
                    spdlog::info("[Server] Player {} attacking", connection->id);

                    connection->player.attack();
                }
                break;
                case ch::message_type::CHANGE_MAP:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} changing map to {}", connection->id, message->id);

                    connection->player.map_index = message->id;
                }
                break;
                case ch::message_type::START_CONVERSATION:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} starting conversation {}", connection->id, message->id);

                    connection->player.start_conversation(world, message->id);
                }
                break;
                case ch::message_type::ADVANCE_CONVERSATION:
                {
                    spdlog::info("[Server] Player {} advancing conversation", connection->id);

                    connection->player.advance_conversation();
                }
                break;
                case ch::message_type::CHOOSE_CONVERSATION_RESPONSE:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} choosing conversation response {}", connection->id, message->id);

                    connection->player.choose_conversation_response(message->id);
                }
                break;
                case ch::message_type::END_CONVERSATION:
                {
                    spdlog::info("[Server] Player {} ending conversation", connection->id);

                    connection->player.end_conversation();
                }
                break;
                case ch::message_type::QUEST_STATUS:
                {
                    const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                    spdlog::info("[Server] Player {} requesting to change quest {} to stage {}", connection->id, message->status.quest_index, message->status.stage_index);

                    connection->player.set_quest_status(message->status);
                }
                break;
                default:
                {
                    spdlog::error("[Server] Unknown message type {}", static_cast<int>(type));
                }
                break;
                }

                enet_packet_destroy(event.packet);
            }
            break;
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                const auto connection = static_cast<ch::connection *>(event.peer->data);

                spdlog::info("[Server] Player {} disconnected", connection->id);

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
