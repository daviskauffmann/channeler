#include "menu.hpp"

#include "../../display.hpp"
#include "../../font.hpp"
#include "../../texture.hpp"
#include "../game/game.hpp"
#include <SDL2/SDL_image.h>
#include <spdlog/spdlog.h>

constexpr const char *server_hostname = "127.0.0.1";
constexpr std::uint16_t server_port = 8492;

template <typename... Args>
auto button(
    const ch::texture *const texture,
    const ch::font *const font,
    const int x,
    const int y,
    std::uint32_t mouse,
    const int mouse_x,
    const int mouse_y,
    const std::string &fmt,
    Args... args)
{
    const int w = 64 * 2;
    const int h = 20 * 2;
    const SDL_Rect dstrect = {x, y, w, h};
    const SDL_Point mouse_point = {mouse_x, mouse_y};

    // if (SDL_PointInRect(&mouse_point, &dstrect))
    // {
    //     SDL_SetTextureColorMod(texture, 128, 128, 128);
    // }
    // else
    // {
    //     SDL_SetTextureColorMod(texture, 255, 255, 255);
    // }

    texture->render(nullptr, &dstrect);
    font->render(x + 30, y + 6, 0, {0, 0, 0}, fmt, std::forward<Args>(args)...);

    return (mouse & SDL_BUTTON_LEFT) && SDL_PointInRect(&mouse_point, &dstrect);
}

ch::menu::menu(std::shared_ptr<ch::display> display)
    : ch::scene(display)
{
    const auto renderer = display->get_renderer();

    font = std::make_unique<ch::font>(renderer, "assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
    button_texture = std::make_unique<ch::texture>(renderer, "assets/NinjaAdventure/HUD/Dialog/ChoiceBox.png");
}

ch::scene *ch::menu::handle_event(const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
        {
            return exit_game();
        }
        break;
        case SDLK_1:
        {
            try
            {
                return change_scene<ch::game>(display, server_hostname, server_port, true);
            }
            catch (const std::exception &e)
            {
                spdlog::error(e.what());

                error_message = e.what();
            }
        }
        break;
        case SDLK_2:
        {
            try
            {
                return change_scene<ch::game>(display, server_hostname, server_port, false);
            }
            catch (const std::exception &e)
            {
                spdlog::error(e.what());

                error_message = e.what();
            }
        }
        break;
        }
    }
    break;
    }

    return this;
}

ch::scene *ch::menu::update(
    const float,
    const std::uint8_t *const,
    const std::uint32_t mouse,
    const int mouse_x,
    const int mouse_y)
{
    font->render(0, 18 * 0, 0, {255, 255, 255}, "Press 1 to host");
    font->render(0, 18 * 1, 0, {255, 255, 255}, "Press 2 to join");
    font->render(0, 18 * 2, 0, {255, 255, 255}, "Press ESC to exit");

    if (!error_message.empty())
    {
        font->render(0, 18 * 4, 0, {255, 0, 0}, error_message);
    }

    if (false)
    {
        if (button(button_texture.get(), font.get(), 100, 100, mouse, mouse_x, mouse_y, "Button 1"))
        {
            spdlog::info("Button 1 Clicked");
        }

        if (button(button_texture.get(), font.get(), 100, 200, mouse, mouse_x, mouse_y, "Button 2"))
        {
            spdlog::info("Button 2 Clicked");
        }

        if (button(button_texture.get(), font.get(), 100, 300, mouse, mouse_x, mouse_y, "Button 3"))
        {
            spdlog::info("Button 3 Clicked");
        }
    }

    return this;
}
