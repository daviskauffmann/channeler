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

        PLAYER_JOINED,
        PLAYER_DISCONNECTED,

        INPUT,
        ATTACK,
        CHANGE_MAP,
        START_CONVERSATION,
        ADVANCE_CONVERSATION,
        CHOOSE_CONVERSATION_RESPONSE,
        END_CONVERSATION,

        QUEST_STATUS,

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

    struct message_quest_status : message
    {
        std::size_t id;
        ch::quest_status status;
    };

    struct message_game_state : message
    {
        struct connection
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

                bool in_conversation;
                std::size_t conversation_root_index;
                std::size_t conversation_node_index;
            } player;
        };

        std::array<connection, ch::server::max_connections> connections;
    };
}

#endif
