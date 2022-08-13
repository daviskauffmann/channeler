#include "display.hpp"

#include <stdexcept>

ch::display::display(const int width, const int height)
    : width(width),
      height(height)
{
    if (SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer) != 0)
    {
        throw std::runtime_error(SDL_GetError());
    }
}

ch::display::~display()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

std::size_t ch::display::get_width() const
{
    return width;
}

std::size_t ch::display::get_height() const
{
    return height;
}

SDL_Renderer *ch::display::get_renderer() const
{
    return renderer;
}

void ch::display::set_title(const char *title) const
{
    SDL_SetWindowTitle(window, title);
}

void ch::display::set_vsync(bool enabled) const
{
    SDL_RenderSetVSync(renderer, enabled);
}

void ch::display::toggle_fullscreen() const
{
    const auto flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void ch::display::handle_event(const SDL_Event &event) const
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_RETURN:
        {
            if (event.key.keysym.mod & KMOD_ALT)
            {
                toggle_fullscreen();
            }
        }
        break;
        }
    }
    break;
    }
}

void ch::display::clear() const
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void ch::display::present() const
{
    SDL_RenderPresent(renderer);
}
