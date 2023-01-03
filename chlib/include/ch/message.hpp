#ifndef CH_MESSAGE_HPP
#define CH_MESSAGE_HPP

#include "server.hpp"
#include <array>

namespace ch
{
    enum class message_type
    {
        server_joined,
        server_full,

        player_connected,
        player_disconnected,

        input,
        attack,
        change_map,
        start_conversation,
        advance_conversation,
        choose_conversation_response,
        end_conversation,

        quest_status,

        game_state
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
        int8_t input_x;
        int8_t input_y;
    };

    struct message_quest_status : message
    {
        std::size_t id;
        ch::quest_status status;
    };

    struct message_game_state : message
    {
        struct player
        {
            std::size_t id;

            std::size_t map_index;

            float position_x;
            float position_y;

            ch::direction direction;
            ch::animation animation;
            std::size_t frame_index;

            bool in_conversation;
            std::size_t conversation_root_index;
            std::size_t conversation_node_index;
        };

        std::array<player, ch::server::max_players> players;
    };
}

#endif
