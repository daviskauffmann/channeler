#include "font.hpp"

#include <stdexcept>

ch::font::font(const char *const file, const int size)
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
