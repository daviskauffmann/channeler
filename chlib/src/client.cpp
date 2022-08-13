#include <ch/client.hpp>

#include <ch/conversation.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <enet/enet.h>
#include <spdlog/spdlog.h>

ch::client::client(const char *const hostname, const std::uint16_t port, ch::world &world)
    : hostname(hostname),
      port(port),
      world(world)
{
    connections.fill(
        {.id = ch::server::max_connections});
}

bool ch::client::is_connected() const
{
    return connected;
}

bool ch::client::connect()
{
    host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host)
    {
        spdlog::error("[Client] Failed to create host");

        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, hostname);
    address.port = port;
    peer = enet_host_connect(host, &address, 2, 0);
    if (!peer)
    {
        spdlog::error("[Client] Failed to create peer");

        enet_host_destroy(host);
        host = nullptr;

        return false;
    }

    std::string failure_reason = "Host timeout";

    ENetEvent event;
    while (enet_host_service(host, &event, 3000) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_CONNECT)
        {
            spdlog::info("[Client] Connected to server {}:{}", hostname, port);
        }
        else if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

            if (type == ch::message_type::SERVER_JOINED)
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                spdlog::info("[Client] Successfully joined with ID {}", message->id);

                connected = true;
                connection_id = message->id;
                connections.at(connection_id).id = connection_id;
            }
            else if (type == ch::message_type::SERVER_FULL)
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
        spdlog::error("[Client] Failed to connect to server: {}", failure_reason);

        enet_peer_reset(peer);
        peer = nullptr;

        enet_host_destroy(host);
        host = nullptr;
    }

    return connected;
}

bool ch::client::disconnect()
{
    enet_peer_disconnect(peer, 0);

    ENetEvent event;
    while (enet_host_service(host, &event, 3000) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            enet_packet_destroy(event.packet);
        }
        else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            spdlog::info("[Client] Successfully disconnected");

            connected = false;
            connection_id = ch::server::max_connections;

            break;
        }
    }

    if (connected)
    {
        spdlog::error("[Client] Server did not confirm disconnect");

        enet_peer_reset(peer);
    }

    peer = nullptr;

    enet_host_destroy(host);
    host = nullptr;

    return !connected;
}

void ch::client::update(const float)
{
    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
        {
            const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

            switch (type)
            {
            case ch::message_type::PLAYER_JOINED:
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                spdlog::info("[Client] Player {} connected", message->id);

                connections.at(message->id).id = message->id;
            }
            break;
            case ch::message_type::PLAYER_DISCONNECTED:
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                spdlog::info("[Client] Player {} disconnected", message->id);

                connections.at(message->id).id = ch::server::max_connections;
            }
            break;
            case ch::message_type::QUEST_STATUS:
            {
                const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                spdlog::info("[Client] Player {} has advanced quest {} to state {}", message->id, message->status.quest_index, message->status.stage_index);

                connections.at(message->id).player.set_quest_status(message->status);
            }
            break;
            case ch::message_type::GAME_STATE:
            {
                const auto message = reinterpret_cast<ch::message_game_state *>(event.packet->data);

                for (std::size_t i = 0; i < message->connections.size(); i++)
                {
                    connections.at(i).id = message->connections.at(i).id;

                    connections.at(i).player.map_index = message->connections.at(i).player.map_index;

                    connections.at(i).player.pos_x = message->connections.at(i).player.pos_x;
                    connections.at(i).player.pos_y = message->connections.at(i).player.pos_y;

                    connections.at(i).player.direction = message->connections.at(i).player.direction;
                    connections.at(i).player.animation = message->connections.at(i).player.animation;
                    connections.at(i).player.frame_index = message->connections.at(i).player.frame_index;

                    if (message->connections.at(i).player.in_conversation)
                    {
                        connections.at(i).player.conversation_root = &world.conversations.at(message->connections.at(i).player.conversation_root_index);
                        connections.at(i).player.conversation_node = connections.at(i).player.conversation_root->find_by_node_index(message->connections.at(i).player.conversation_node_index);
                    }
                    else
                    {
                        connections.at(i).player.conversation_root = nullptr;
                        connections.at(i).player.conversation_node = nullptr;
                    }
                }
            }
            break;
            default:
            {
                spdlog::error("[Client] Unknown message type {}", static_cast<int>(type));
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

void ch::client::send(ENetPacket *packet) const
{
    // TODO: handle errors?
    enet_peer_send(peer, 0, packet);
}

const ch::player &ch::client::get_player() const
{
    return connections.at(connection_id).player;
}
