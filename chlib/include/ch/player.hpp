#ifndef CH_PLAYER_HPP
#define CH_PLAYER_HPP

#include <functional>
#include <vector>

namespace ch
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
        ATTACKING
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

        bool attacking = false;
        float attack_timer = 0;

        ch::direction direction = ch::direction::DOWN;
        ch::animation animation = ch::animation::IDLE;
        float animation_timer = 0;
        std::size_t frame_index = 0;

        ch::conversation *conversation_root = nullptr;
        ch::conversation *conversation_node = nullptr;

        std::vector<ch::quest_status> quest_statuses;
        std::function<void(const ch::quest_status &)> on_quest_status_set;

        void update(const ch::input &input, const ch::map &map, float delta_time);

        void attack();

        void start_conversation(const ch::world &world, std::size_t root_index);
        void advance_conversation();
        void choose_conversation_response(std::size_t choice_index);
        void end_conversation();

        void set_quest_status(const ch::quest_status &status);
        bool check_quest_status(const ch::quest_status &status) const;
    };
}

#endif
