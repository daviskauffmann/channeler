#include <ch/client.hpp>

#include <ch/conversation.hpp>
#include <ch/message.hpp>
#include <ch/world.hpp>
#include <iostream>

ch::client::client(ch::world &world)
    : world(world)
{
    connections.fill(
        {.id = ch::server::max_connections});

    host = enet_host_create(nullptr, 1, 2, 0, 0);
}

ch::client::~client()
{
    listening = false;
    listen_thread.join();

    {
        enet_peer_disconnect(peer, 0);

        auto disconnected = false;

        ENetEvent event;
        while (enet_host_service(host, &event, 3000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
            {
                std::cout << "[Client] Disconnected" << std::endl;

                disconnected = true;

                break;
            }
        }

        if (!disconnected)
        {
            std::cout << "[Client] Server did not confirm disconnect" << std::endl;

            enet_peer_reset(peer);
        }
    }

    enet_host_destroy(host);
}

bool ch::client::connect(const char *const hostname, const std::uint16_t port)
{
    ENetAddress address;
    enet_address_set_host(&address, hostname);
    address.port = port;
    peer = enet_host_connect(host, &address, 2, 0);
    if (!peer)
    {
        return false;
    }

    bool connected = false;

    ENetEvent event;
    while (enet_host_service(host, &event, 3000) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_CONNECT)
        {
            std::cout << "[Client] Connected to " << hostname << ":" << port << std::endl;
        }
        else if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            const auto type = reinterpret_cast<ch::message *>(event.packet->data)->type;

            if (type == ch::message_type::SERVER_JOINED)
            {
                const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                std::cout << "[Client] Assigned ID " << message->id << std::endl;

                connected = true;
                connection_id = message->id;
                connections.at(connection_id).id = connection_id;

                break;
            }
            else if (type == ch::message_type::SERVER_FULL)
            {
                std::cout << "[Client] Server full" << std::endl;
            }
            else
            {
                std::cout << "[Client] Unknown server response" << std::endl;
            }

            enet_packet_destroy(event.packet);
        }
    }

    if (!connected)
    {
        std::cout << "[Client] Could not connect to server" << std::endl;

        enet_peer_reset(peer);

        return false;
    }

    std::cout << "[Client] Successfully joined server" << std::endl;
    listen_thread = std::thread(&ch::client::listen, this);

    return true;
}

void ch::client::send(ENetPacket *packet)
{
    // TODO: handle errors?
    enet_peer_send(peer, 0, packet);
}

ch::player &ch::client::get_player()
{
    return connections.at(connection_id).player;
}

void ch::client::listen()
{
    while (listening)
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

                    std::cout << "[Client] Player " << message->id << " connected" << std::endl;

                    connections.at(message->id).id = message->id;
                }
                break;
                case ch::message_type::PLAYER_DISCONNECTED:
                {
                    const auto message = reinterpret_cast<ch::message_id *>(event.packet->data);

                    std::cout << "[Client] Player " << message->id << " disconnected" << std::endl;

                    connections.at(message->id).id = ch::server::max_connections;
                }
                break;
                case ch::message_type::QUEST_STATUS:
                {
                    const auto message = reinterpret_cast<ch::message_quest_status *>(event.packet->data);

                    std::cout << "[Client] Player " << message->id << " has advanced quest " << message->status.quest_index << " to stage " << message->status.stage_index << std::endl;

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
                            connections.at(i).player.conversation_root = world.conversations.at(message->connections.at(i).player.conversation_root_index);
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
                    std::cout << "[Client] Unknown message type " << static_cast<int>(type) << std::endl;
                }
                break;
                }

                enet_packet_destroy(event.packet);
            }
            break;
            }
        }
    }
}
