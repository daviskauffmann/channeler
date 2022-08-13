#ifndef CH_MENU_SCENE_HPP
#define CH_MENU_SCENE_HPP

#include "../scene.hpp"
#include <SDL2/SDL.h>
#include <memory>

namespace ch
{
    class font;
    class texture;

    class menu_scene : public ch::scene
    {
    public:
        explicit menu_scene(const ch::display &display);

        void handle_event(const SDL_Event &event) override;
        void update(
            float delta_time,
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y) override;

    private:
        std::unique_ptr<ch::font> font;
        std::unique_ptr<ch::texture> button_texture;
    };
}

#endif
