#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include "../scene.hpp"
#include <SDL2/SDL_ttf.h>
#include <cstddef>

namespace ch
{
    class client;
    class server;
    class world;
    class active_map;

    class game_scene : public ch::scene
    {
    public:
        game_scene(SDL_Renderer *renderer, const char *hostname, std::uint16_t port, bool is_host);
        ~game_scene() override;

        ch::scene *handle_event(const SDL_Event &event) override;
        ch::scene *update(
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y,
            float delta_time) override;

    private:
        TTF_Font *font;
        ch::world *world;
        ch::server *server = nullptr;
        ch::client *client;
        ch::active_map *active_map;
        std::size_t map_index;
        SDL_Texture *player_idle_sprites;
        SDL_Texture *player_walk_sprites;
        SDL_Texture *player_attack_sprites;
        bool quest_log_open = false;
    };
}

#endif
