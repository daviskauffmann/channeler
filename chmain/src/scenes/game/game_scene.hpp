#ifndef CH_SCENES_GAME_SCENE_HPP
#define CH_SCENES_GAME_SCENE_HPP

#include "../../scene.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace ch
{
    struct active_map;
    struct loaded_item;
    class client;
    class font;
    class server;
    class sound;
    class texture;
    class world;

    class game_scene : public ch::scene
    {
    public:
        game_scene(
            std::shared_ptr<ch::display> display,
            const char *hostname,
            std::uint16_t port,
            bool is_host);

        ch::scene *handle_event(const SDL_Event &event) override;
        ch::scene *update(
            float delta_time,
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y) override;

    private:
        std::unique_ptr<ch::font> font;
        std::shared_ptr<ch::world> world;
        std::unique_ptr<ch::server> server;
        std::unique_ptr<ch::client> client;
        std::unique_ptr<ch::active_map> active_map;
        std::vector<std::unique_ptr<ch::loaded_item>> loaded_items;
        std::size_t map_index;
        std::size_t weapon_item_index;
        std::unique_ptr<ch::texture> player_spritesheet;
        std::unique_ptr<ch::texture> dialog_box;
        bool quest_log_open = false;
        bool left_panel_open = false;
        std::int8_t input_x = 0;
        std::int8_t input_y = 0;
    };
}

#endif
