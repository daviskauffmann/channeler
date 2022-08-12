#include "menu_scene.hpp"

#include "../draw_text.hpp"
#include "game_scene.hpp"
#include <SDL2/SDL_image.h>
#include <spdlog/spdlog.h>

constexpr const char *server_hostname = "127.0.0.1";
constexpr std::uint16_t server_port = 8492;

template <typename... Args>
bool button(
    SDL_Renderer *const renderer,
    SDL_Texture *const texture,
    TTF_Font *font,
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

    if (SDL_PointInRect(&mouse_point, &dstrect))
    {
        SDL_SetTextureColorMod(texture, 128, 128, 128);
    }
    else
    {
        SDL_SetTextureColorMod(texture, 255, 255, 255);
    }

    SDL_RenderCopy(renderer, texture, nullptr, &dstrect);
    draw_text(renderer, font, x + 30, y + 6, 0, {0, 0, 0}, fmt, std::forward<Args>(args)...);

    return (mouse & SDL_BUTTON_LEFT) && SDL_PointInRect(&mouse_point, &dstrect);
}

ch::menu_scene::menu_scene(SDL_Renderer *const renderer)
    : scene(renderer)
{
    font = TTF_OpenFont("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
    choice_box = IMG_LoadTexture(renderer, "assets/NinjaAdventure/HUD/Dialog/ChoiceBox.png");
}

ch::menu_scene::~menu_scene()
{
    TTF_CloseFont(font);
    SDL_DestroyTexture(choice_box);
}

ch::scene *ch::menu_scene::handle_event(const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
        {
            delete this;
            return nullptr;
        }
        break;
        case SDLK_1:
        {
            try
            {
                const auto scene = new ch::game_scene(renderer, server_hostname, server_port, true);
                delete this;
                return scene;
            }
            catch (const std::exception &e)
            {
                spdlog::error(e.what());
            }
        }
        break;
        case SDLK_2:
        {
            try
            {
                const auto scene = new ch::game_scene(renderer, server_hostname, server_port, false);
                delete this;
                return scene;
            }
            catch (const std::exception &e)
            {
                spdlog::error(e.what());
            }
        }
        break;
        }
    }
    break;
    }

    return this;
}

ch::scene *ch::menu_scene::update(
    const std::uint8_t *const,
    const std::uint32_t mouse,
    const int mouse_x,
    const int mouse_y,
    const float)
{
    draw_text(renderer, font, 0, 18 * 0, 0, {255, 255, 255}, "Press 1 to host");
    draw_text(renderer, font, 0, 18 * 1, 0, {255, 255, 255}, "Press 2 to join");
    draw_text(renderer, font, 0, 18 * 2, 0, {255, 255, 255}, "Press ESC to exit");

    if (button(renderer, choice_box, font, 100, 100, mouse, mouse_x, mouse_y, "Button 1"))
    {
        spdlog::info("Button 1 Clicked");
    }

    if (button(renderer, choice_box, font, 100, 200, mouse, mouse_x, mouse_y, "Button 2"))
    {
        spdlog::info("Button 2 Clicked");
    }

    if (button(renderer, choice_box, font, 100, 300, mouse, mouse_x, mouse_y, "Button 3"))
    {
        spdlog::info("Button 3 Clicked");
    }

    return this;
}
