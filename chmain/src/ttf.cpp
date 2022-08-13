#include "ttf.hpp"

#include <SDL2/SDL_ttf.h>
#include <stdexcept>

ch::ttf::ttf()
{
    if (TTF_Init() != 0)
    {
        throw std::runtime_error(TTF_GetError());
    }
}

ch::ttf::~ttf()
{
    TTF_Quit();
}
