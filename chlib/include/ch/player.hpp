#ifndef CH_PLAYER_HPP
#define CH_PLAYER_HPP

#include "vec2.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace ch
{
    struct conversation;
    struct input;
    struct map;
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
        std::size_t map_index = 0;

        ch::vec2 position = {100, 100};
        ch::vec2 velocity = {0, 0};
        ch::vec2 acceleration = {0, 0};

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

        void update(const ch::input &input, const ch::map &map, float delta_time);

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
