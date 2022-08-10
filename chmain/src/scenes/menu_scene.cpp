#include "menu_scene.hpp"

#include "../draw_text.hpp"
#include "game_scene.hpp"
#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

constexpr const char *server_hostname = "127.0.0.1";
constexpr std::uint16_t server_port = 8492;

ch::menu_scene::menu_scene(SDL_Renderer *const renderer)
    : scene(renderer)
{
    font = TTF_OpenFont("assets/NinjaAdventure/HUD/Font/NormalFont.ttf", 18);
}

ch::menu_scene::~menu_scene()
{
    TTF_CloseFont(font);
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
    const std::uint32_t,
    const int,
    const int,
    const float)
{
    draw_text(renderer, font, 0, 18 * 0, 200, {255, 255, 255}, "Press 1 to host");
    draw_text(renderer, font, 0, 18 * 1, 200, {255, 255, 255}, "Press 2 to join");
    draw_text(renderer, font, 0, 18 * 2, 200, {255, 255, 255}, "Press ESC to exit");

    return this;
}
