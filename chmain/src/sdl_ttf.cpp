#include "sdl_ttf.hpp"

#include <SDL2/SDL_ttf.h>
#include <stdexcept>

ch::sdl_ttf::sdl_ttf()
{
    if (TTF_Init() != 0)
    {
        throw std::runtime_error(TTF_GetError());
    }
}

ch::sdl_ttf::~sdl_ttf()
{
    TTF_Quit();
}
