#ifndef CH_GAME_SCENE_HPP
#define CH_GAME_SCENE_HPP

#include "../scene.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>

namespace ch
{
    class active_map;
    class client;
    class font;
    class server;
    class texture;
    class world;

    class game_scene : public ch::scene
    {
    public:
        game_scene(std::shared_ptr<ch::display> display, const char *hostname, std::uint16_t port, bool is_host);

        void handle_event(const SDL_Event &event) override;
        void update(
            float delta_time,
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y) override;

    private:
        std::unique_ptr<ch::font> font;
        std::unique_ptr<ch::world> world;
        std::unique_ptr<ch::server> server;
        std::unique_ptr<ch::client> client;
        std::unique_ptr<ch::active_map> active_map;
        std::size_t map_index;
        std::unique_ptr<ch::texture> player_idle_sprites;
        std::unique_ptr<ch::texture> player_walk_sprites;
        std::unique_ptr<ch::texture> player_attack_sprites;
        bool quest_log_open = false;
    };
}

#endif
