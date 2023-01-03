#include "font.hpp"

#include <stdexcept>

ch::font::font(SDL_Renderer *const renderer, const char *const file, const int size)
    : renderer(renderer)
{
    ttf_font = TTF_OpenFont(file, size);
    if (!ttf_font)
    {
        throw std::runtime_error(TTF_GetError());
    }
}

ch::font::~font()
{
    TTF_CloseFont(ttf_font);
}
