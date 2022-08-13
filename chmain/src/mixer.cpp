#include "mixer.hpp"

#include <SDL2/SDL_mixer.h>
#include <stdexcept>

ch::mixer::mixer()
{
    constexpr int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        throw std::runtime_error(Mix_GetError());
    }
}

ch::mixer::~mixer()
{
    Mix_Quit();
}
