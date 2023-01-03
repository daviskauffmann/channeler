#include <ch/server.hpp>

#include <algorithm>
#include <ch/conversation.hpp>
#include <ch/host.hpp>
#include <ch/map.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <spdlog/spdlog.h>

ch::server::server(
    const std::uint16_t port,
    const std::shared_ptr<ch::world> world)
    : world(world)
{
    players.fill(
        {.id = max_players});

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    host = std::make_unique<ch::host>(&address, max_players, 2, 0, 0);

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
    for (auto &player : players)
    {
        if (player.id != max_players)
        {
            player.update(delta_time);
        }
    }

    world->update(delta_time);

    for (auto &player : players)
    {
        if (player.id != max_players)
        {
            const auto position = player.body->GetPosition();

            player.position_x = position.x;
            player.position_y = position.y;
        }
    }

    {
        ch::message_game_state message;
        message.type = ch::message_type::game_state;
        for (std::size_t i = 0; i < players.size(); i++)
        {
            message.players.at(i).id = players.at(i).id;

            message.players.at(i).map_index = players.at(i).map_index;

            message.players.at(i).position_x = players.at(i).position_x;
            message.players.at(i).position_y = players.at(i).position_y;

            message.players.at(i).direction = players.at(i).direction;
            message.players.at(i).animation = players.at(i).animation;
            message.players.at(i).frame_index = players.at(i).frame_index;

            if (players.at(i).conversation_node)
            {
                message.players.at(i).in_conversation = true;
                message.players.at(i).conversation_root_index = players.at(i).conversation_node->root_index;
                message.players.at(i).conversation_node_index = players.at(i).conversation_node->node_index;
            }
            else
            {
                message.players.at(i).in_conversation = false;
            }
        }

        const auto packet = enet_packet_create(&message, sizeof(message), 0);
        host->broadcast(packet);
    }
}

void ch::server::listen()
{
    b2BodyDef body_def;
    body_def.type = b2_dynamicBody;
    body_def.position.Set(100.0f, 100.0f);

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(1.0f, 1.0f);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;

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

                const auto new_player = std::find_if(
                    players.begin(),
                    players.end(),
                    [](const auto &player)
                    {
                        return player.id == max_players;
                    });
                if (new_player != players.end())
                {
                    new_player->id = std::distance(players.begin(), new_player);
                    new_player->map_index = 0;
                    new_player->body = world->maps.at(new_player->map_index).b2_world->CreateBody(&body_def);
                    new_player->body->CreateFixture(&fixtureDef);
                    new_player->on_quest_status_set = [this, new_player](const ch::quest_status &status)
                    {
                        spdlog::info("[Server] Player {} has advanced quest {} to stage {}", new_player->id, status.quest_index, status.stage_index);

                        ch::message_quest_status message;
                        message.type = ch::message_type::quest_status;
                        message.id = new_player->id;
                        message.status = status;
                        const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        host->broadcast(packet);
                    };
                    event.peer->data = &*new_player;

                    spdlog::info("[Server] Assigned ID {}", new_player->id);

                    {
                        ch::message_id message;
                        message.type = ch::message_type::server_joined;
                        message.id = new_player->id;
                        const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }

                    for (const auto &player : players)
                    {
                        for (const auto &quest_status : player.quest_statuses)
                        {
                            ch::message_quest_status message;
                            message.type = ch::message_type::quest_status;
                            message.id = player.id;
                            message.status = quest_status;
                            const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, 0, packet);
                        }
                    }

                    {
                        ch::message_id message;
                        message.type = ch::message_type::player_connected;
                        message.id = new_player->id;
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
                const auto player = static_cast<ch::player *>(event.peer->data);
                const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

                switch (type)
                {
                case ch::message_type::input:
                {
                    const auto message = reinterpret_cast<ch::message_input *>(event.packet->data);

                    player->input_x = message->input_x;
                    player->input_y = message->input_y;
                }
                break;
                case ch::message_type::attack:
                {
                    spdlog::info("[Server] Player {} attacking", player->id);

                    player->attack();
                }
                break;
                case ch::message_type::change_map:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} changing map to {}", player->id, message->id);

                    world->maps.at(player->map_index).b2_world->DestroyBody(player->body);
                    player->map_index = message->id;
                    player->body = world->maps.at(player->map_index).b2_world->CreateBody(&body_def);
                    player->body->CreateFixture(&fixtureDef);
                }
                break;
                case ch::message_type::start_conversation:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} starting conversation {}", player->id, message->id);

                    player->start_conversation(world, message->id);
                }
                break;
                case ch::message_type::advance_conversation:
                {
                    spdlog::info("[Server] Player {} advancing conversation", player->id);

                    player->advance_conversation();
                }
                break;
                case ch::message_type::choose_conversation_response:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    spdlog::info("[Server] Player {} choosing conversation response {}", player->id, message->id);

                    player->choose_conversation_response(message->id);
                }
                break;
                case ch::message_type::end_conversation:
                {
                    spdlog::info("[Server] Player {} ending conversation", player->id);

                    player->end_conversation();
                }
                break;
                case ch::message_type::quest_status:
                {
                    const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                    spdlog::info("[Server] Player {} requesting to change quest {} to stage {}", player->id, message->status.quest_index, message->status.stage_index);

                    player->set_quest_status(message->status);
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
                const auto player = static_cast<ch::player *>(event.peer->data);

                spdlog::info("[Server] Player {} disconnected", player->id);

                world->maps.at(player->map_index).b2_world->DestroyBody(player->body);

                {
                    ch::message_id message;
                    message.type = ch::message_type::player_disconnected;
                    message.id = player->id;
                    const auto packet = enet_packet_create(&message, sizeof(message), ENET_PACKET_FLAG_RELIABLE);
                    host->broadcast(packet);
                }

                {
                    player->id = max_players;
                    event.peer->data = nullptr;
                }
            }
            break;
            }
        }
    }
}
