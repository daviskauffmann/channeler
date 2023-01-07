#ifndef CH_MENU_HPP
#define CH_MENU_HPP

#include "../../scene.hpp"
#include <SDL2/SDL.h>
#include <string>

namespace ch
{
    class font;
    class music;
    class texture;

    class menu_scene : public ch::scene
    {
    public:
        explicit menu_scene(std::shared_ptr<ch::display> display);

        ch::scene *handle_event(const SDL_Event &event) override;
        ch::scene *update(
            float delta_time,
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y) override;

    private:
        std::unique_ptr<ch::font> font;
        std::unique_ptr<ch::texture> button_texture;
        std::unique_ptr<ch::music> music;

        std::string error_message = "";
    };
}

#endif
