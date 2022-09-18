#include <ch/server.hpp>

#include <algorithm>
#include <ch/conversation.hpp>
#include <ch/host.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <spdlog/spdlog.h>

ch::server::server(
    const std::uint16_t port,
    const std::shared_ptr<ch::world> world)
    : world(world)
{
    connections.fill(
        {.id = max_connections});

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    host = std::make_unique<ch::host>(&address, max_connections, 2, 0, 0);

    listening = true;
    listen_thread = std::thread(&ch::server::listen, this);

    spdlog::info("[Server] Started on port {}", port);
}

ch::server::~server()
{
    listening = false;
    listen_thread.join();

    spdlog::info("[Server] Successfully stopped");
}

void ch::server::handle_event(const SDL_Event &)
{
}

void ch::server::update(const float delta_time)
{
    for (auto &connection : connections)
    {
        if (connection.id != max_connections)
        {
            connection.player.update(connection.input, world->maps.at(connection.player.map_index), delta_time);
        }
    }

    {
        ch::message_game_state message;
        message.type = ch::message_type::game_state;
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

        const auto packet = enet_packet_create(&message, sizeof(message), 0);
        host->broadcast(packet);
    }
}

void ch::server::listen()
{
    while (listening)
    {
        ENetEvent event;
        while (host->service(&event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                spdlog::info("[Server] Player connected {}:{}", event.peer->address.host, event.peer->address.port);

                const auto new_connection = std::find_if(
                    connections.begin(),
                    connections.end(),
                    [](const auto &connection)
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
                        message.type = ch::message_type::quest_status;
                        message.id = new_connection->id;
                        message.status = status;
                        const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        host->broadcast(packet);
                    };
                    event.peer->data = &*new_connection;

                    spdlog::info("[Server] Assigned ID {}", new_connection->id);

                    {
                        ch::message_id message;
                        message.type = ch::message_type::server_joined;
                        message.id = new_connection->id;
                        const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }

                    for (const auto &connection : connections)
                    {
                        for (const auto &quest_status : connection.player.quest_statuses)
                        {
                            ch::message_quest_status message;
                            message.type = ch::message_type::quest_status;
                            message.id = connection.id;
                            message.status = quest_status;
                            const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, 0, packet);
                        }
                    }

                    {
                        ch::message_id message;
                        message.type = ch::message_type::player_joined;
                        message.id = new_connection->id;
                        const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        host->broadcast(packet);
                    }
                }
                else
                {
                    spdlog::warn("[Server] Player tried to join, but server is full");

                    ch::message message;
                    message.type = ch::message_type::server_full;
                    const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
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
                case ch::message_type::input:
                {
                    const auto message = reinterpret_cast<ch::message_input *>(event.packet->data);

                    connection->input.dx = message->input.dx;
                    connection->input.dy = message->input.dy;
                }
                break;
                case ch::message_type::attack:
                {
                    spdlog::info("[Server] Player {} attacking", connection->id);

                    connection->player.attack();
                }
                break;
                case ch::message_type::change_map:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} changing map to {}", connection->id, message->id);

                    connection->player.map_index = message->id;
                }
                break;
                case ch::message_type::start_conversation:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} starting conversation {}", connection->id, message->id);

                    connection->player.start_conversation(world, message->id);
                }
                break;
                case ch::message_type::advance_conversation:
                {
                    spdlog::info("[Server] Player {} advancing conversation", connection->id);

                    connection->player.advance_conversation();
                }
                break;
                case ch::message_type::choose_conversation_response:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} choosing conversation response {}", connection->id, message->id);

                    connection->player.choose_conversation_response(message->id);
                }
                break;
                case ch::message_type::end_conversation:
                {
                    spdlog::info("[Server] Player {} ending conversation", connection->id);

                    connection->player.end_conversation();
                }
                break;
                case ch::message_type::quest_status:
                {
                    const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                    spdlog::info("[Server] Player {} requesting to change quest {} to stage {}", connection->id, message->status.quest_index, message->status.stage_index);

                    connection->player.set_quest_status(message->status);
                }
                break;
                default:
                {
                    spdlog::warn("[Server] Unknown message type {}", static_cast<int>(type));
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
                    message.type = ch::message_type::player_disconnected;
                    message.id = connection->id;
                    const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                    host->broadcast(packet);
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
