#include "menu_scene.hpp"

#include "../display.hpp"
#include "../font.hpp"
#include "../texture.hpp"
#include "game_scene.hpp"
#include <SDL2/SDL_image.h>
#include <spdlog/spdlog.h>

constexpr const char *server_hostname = "127.0.0.1";
constexpr std::uint16_t server_port = 8492;

template <typename... Args>
auto button(
    SDL_Renderer *const renderer,
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

    texture->render(renderer, nullptr, &dstrect);
    font->render(renderer, x + 30, y + 6, 0, {0, 0, 0}, fmt, std::forward<Args>(args)...);

    return (mouse & SDL_BUTTON_LEFT) && SDL_PointInRect(&mouse_point, &dstrect);
}

ch::menu_scene::menu_scene(std::shared_ptr<ch::display> display)
    : scene(display)
{
    font = std::make_unique<ch::font>("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
    button_texture = std::make_unique<ch::texture>(display->get_renderer(), "assets/NinjaAdventure/HUD/Dialog/ChoiceBox.png");
}

void ch::menu_scene::handle_event(const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
            return delete_scene();
        case SDLK_1:
            return change_scene<ch::game_scene>(std::ref(display), server_hostname, server_port, true);
        case SDLK_2:
            return change_scene<ch::game_scene>(std::ref(display), server_hostname, server_port, false);
        }
    }
    break;
    }
}

void ch::menu_scene::update(
    float,
    const std::uint8_t *,
    std::uint32_t mouse,
    int mouse_x,
    int mouse_y)
{
    font->render(display->get_renderer(), 0, 18 * 0, 0, {255, 255, 255}, "Press 1 to host");
    font->render(display->get_renderer(), 0, 18 * 1, 0, {255, 255, 255}, "Press 2 to join");
    font->render(display->get_renderer(), 0, 18 * 2, 0, {255, 255, 255}, "Press ESC to exit");

    if (button(display->get_renderer(), button_texture.get(), font.get(), 100, 100, mouse, mouse_x, mouse_y, "Button 1"))
    {
        spdlog::info("Button 1 Clicked");
    }

    if (button(display->get_renderer(), button_texture.get(), font.get(), 100, 200, mouse, mouse_x, mouse_y, "Button 2"))
    {
        spdlog::info("Button 2 Clicked");
    }

    if (button(display->get_renderer(), button_texture.get(), font.get(), 100, 300, mouse, mouse_x, mouse_y, "Button 3"))
    {
        spdlog::info("Button 3 Clicked");
    }
}
