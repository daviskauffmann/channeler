#include "client.hpp"

#include "peer.hpp"
#include <ch/conversation.hpp>
#include <ch/host.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

ch::client::client(
    const char *const hostname,
    const std::uint16_t port,
    const std::shared_ptr<ch::world> world)
    : world(world)
{
    players.fill(
        {.id = ch::server::max_players});

    host = std::make_unique<ch::host>(nullptr, 1, 2, 0, 0);

    ENetAddress address;
    enet_address_set_host(&address, hostname);
    address.port = port;
    peer = std::make_unique<ch::peer>(host->get_enet_host(), &address, 2, 0);

    bool connected = false;
    std::string failure_reason = "Host timeout";

    ENetEvent event;
    while (host->service(&event, 3000) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_CONNECT)
        {
            spdlog::info("[Client] Connected to server {}:{}", hostname, port);
        }
        else if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

            if (type == ch::message_type::server_joined)
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                spdlog::info("[Client] Successfully joined with ID {}", message->id);

                connected = true;
                player_id = message->id;
                players.at(player_id).id = player_id;
            }
            else if (type == ch::message_type::server_full)
            {
                failure_reason = "Server full";
            }
            else
            {
                failure_reason = "Unknown server response";
            }

            enet_packet_destroy(event.packet);

            break;
        }
    }

    if (!connected)
    {
        throw std::runtime_error(fmt::format("Failed to connect to server: {}", failure_reason).c_str());
    }
}

ch::client::~client()
{
    peer->disconnect();

    ENetEvent event;
    while (host->service(&event, 3000) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            enet_packet_destroy(event.packet);
        }
        else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            spdlog::info("[Client] Successfully disconnected");

            peer->mark_successfully_disconnected();

            break;
        }
    }

    if (!peer->is_successfully_disconnected())
    {
        spdlog::warn("[Client] Server did not confirm disconnect");
    }
}

void ch::client::handle_event(const SDL_Event &)
{
}

void ch::client::update(const float)
{
    ENetEvent event;
    while (host->service(&event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
        {
            const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

            switch (type)
            {
            case ch::message_type::player_connected:
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                spdlog::info("[Client] Player {} connected", message->id);

                players.at(message->id).id = message->id;
            }
            break;
            case ch::message_type::player_disconnected:
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                spdlog::info("[Client] Player {} disconnected", message->id);

                players.at(message->id).id = ch::server::max_players;
            }
            break;
            case ch::message_type::quest_status:
            {
                const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                spdlog::info("[Client] Player {} has advanced quest {} to state {}", message->id, message->status.quest_index, message->status.stage_index);

                players.at(message->id).set_quest_status(message->status);
            }
            break;
            case ch::message_type::game_state:
            {
                const auto message = reinterpret_cast<ch::message_game_state *>(event.packet->data);

                for (std::size_t i = 0; i < message->players.size(); i++)
                {
                    players.at(i).id = message->players.at(i).id;

                    players.at(i).map_index = message->players.at(i).map_index;

                    players.at(i).position_x = message->players.at(i).position_x;
                    players.at(i).position_y = message->players.at(i).position_y;

                    players.at(i).direction = message->players.at(i).direction;
                    players.at(i).animation = message->players.at(i).animation;
                    players.at(i).frame_index = message->players.at(i).frame_index;

                    if (message->players.at(i).in_conversation)
                    {
                        players.at(i).conversation_root = &world->conversations.at(message->players.at(i).conversation_root_index);
                        players.at(i).conversation_node = players.at(i).conversation_root->find_by_node_index(message->players.at(i).conversation_node_index);
                    }
                    else
                    {
                        players.at(i).conversation_root = nullptr;
                        players.at(i).conversation_node = nullptr;
                    }
                }
            }
            break;
            default:
            {
                spdlog::warn("[Client] Unknown message type {}", static_cast<int>(type));
            }
            break;
            }

            enet_packet_destroy(event.packet);
        }
        break;
        }
    }

    // TODO: client side prediction?
}

void ch::client::send(const void *const data, const std::size_t length, const std::uint32_t flags) const
{
    const auto packet = enet_packet_create(data, length, flags);
    peer->send(packet);
}

const ch::player &ch::client::get_player() const
{
    return players.at(player_id);
}
