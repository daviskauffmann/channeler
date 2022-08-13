#include "image.hpp"

#include <SDL2/SDL_image.h>
#include <stdexcept>

ch::image::image()
{
    constexpr int img_flags = IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        throw std::runtime_error(IMG_GetError());
    }
}

ch::image::~image()
{
    IMG_Quit();
}
