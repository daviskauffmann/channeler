#ifndef CH_ITEM_HPP
#define CH_ITEM_HPP

#include <array>
#include <nlohmann/json.hpp>
#include <string>

namespace ch
{
    struct attack_position
    {
        float x_offset;
        float y_offset;
        float angle;
    };

    struct item
    {
        std::string name;
        std::string sprite_filename;
        float width;
        float height;

        std::array<ch::attack_position, 4> attack_positions;
        std::string attack_sprite_filename;
        std::string attack_sound_filename;

        item(const nlohmann::json &item_json);
    };
}

#endif
