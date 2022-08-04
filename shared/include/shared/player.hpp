#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "quest_status.hpp"
#include <vector>

namespace hp
{
    class conversation_node;
    class conversations;
    struct input;
    class map;

    enum class direction
    {
        DOWN,
        UP,
        LEFT,
        RIGHT
    };

    enum class animation
    {
        IDLE,
        WALKING,
    };

    class player
    {
    public:
        std::size_t map_index;

        float pos_x = 100;
        float pos_y = 100;
        float vel_x = 0;
        float vel_y = 0;
        float acc_x = 0;
        float acc_y = 0;

        hp::direction direction = hp::direction::DOWN;
        hp::animation animation = hp::animation::IDLE;
        float animation_timer = 0;
        std::size_t frame_index = 0;

        hp::conversation_node *conversation_root = nullptr;
        hp::conversation_node *conversation_node = nullptr;

        std::vector<hp::quest_status> quest_statuses;

        player(std::size_t map_index);

        void update(hp::input *input, hp::map *map, float delta_time);

        void attack();

        void start_conversation(hp::conversations *conversations, size_t root_index);
        void advance_conversation();
        void choose_conversation_response(size_t choice_index);
        void end_conversation();

        void set_quest_status(struct quest_status status);
        bool check_quest_status(struct quest_status status);
    };
}

#endif
