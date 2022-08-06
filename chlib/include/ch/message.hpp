#ifndef CH_MESSAGE_HPP
#define CH_MESSAGE_HPP

#include "server.hpp"
#include <array>

namespace ch
{
    enum class message_type
    {
        SERVER_JOINED,
        SERVER_FULL,

        CLIENT_JOINED,
        CLIENT_DISCONNECTED,

        INPUT,
        CHANGE_MAP,

        GAME_STATE
    };

    struct message
    {
        message_type type;
    };

    struct message_id : message
    {
        std::size_t id;
    };

    struct message_input : message
    {
        ch::input input;
    };

    struct message_game_state : message
    {
        struct client
        {
            std::size_t id;
            struct
            {
                std::size_t map_index;

                float pos_x;
                float pos_y;

                ch::direction direction;
                ch::animation animation;
                std::size_t frame_index;

                std::size_t conversation_root_index;
                std::size_t conversation_node_index;
            } player;
        };

        std::array<client, ch::server::max_clients> clients;
    };
}

#endif
