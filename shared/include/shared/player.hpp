#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <vector>

namespace hp
{
    class conversation;
    struct input;
    class map;
    class world;

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

    struct quest_status
    {
        std::size_t quest_index;
        std::size_t stage_index;
    };

    class player
    {
    public:
        std::size_t map_index = 0;

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

        hp::conversation *conversation_root = nullptr;
        hp::conversation *conversation_node = nullptr;

        std::vector<hp::quest_status> quest_statuses;

        void update(const hp::input &input, const hp::map &map, float delta_time);

        void attack();

        void start_conversation(const hp::world &world, std::size_t root_index);
        void advance_conversation();
        void choose_conversation_response(std::size_t choice_index);
        void end_conversation();

        void set_quest_status(const hp::quest_status &status);
        bool check_quest_status(const hp::quest_status &status) const;
    };
}

#endif
