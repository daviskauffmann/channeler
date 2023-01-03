#ifndef CH_PLAYER_HPP
#define CH_PLAYER_HPP

#include <box2d/box2d.h>
#include <functional>
#include <memory>
#include <vector>

namespace ch
{
    struct conversation;
    class world;

    enum class direction
    {
        down,
        up,
        left,
        right
    };

    enum class animation
    {
        idle,
        walking,
        attacking
    };

    struct quest_status
    {
        std::size_t quest_index;
        std::size_t stage_index;
    };

    struct player
    {
        std::size_t id;

        std::size_t map_index = 0;

        int8_t input_x = 0;
        int8_t input_y = 0;

        b2Body *body = nullptr;
        float position_x;
        float position_y;

        bool attacking = false;
        float attack_timer = 0;

        ch::direction direction = ch::direction::down;
        ch::animation animation = ch::animation::idle;
        float animation_timer = 0;
        std::size_t frame_index = 0;

        const ch::conversation *conversation_root = nullptr;
        const ch::conversation *conversation_node = nullptr;

        std::vector<ch::quest_status> quest_statuses;
        std::function<void(const ch::quest_status &)> on_quest_status_set;

        void update(float delta_time);

        void attack();

        void start_conversation(std::shared_ptr<ch::world> world, std::size_t root_index);
        void advance_conversation();
        void choose_conversation_response(std::size_t choice_index);
        void end_conversation();

        void set_quest_status(const ch::quest_status &status);
        bool check_quest_status(const ch::quest_status &status) const;
    };
}

#endif
