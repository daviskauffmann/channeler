#include "audio.hpp"

#include <SDL2/SDL_mixer.h>
#include <stdexcept>

ch::audio::audio()
{
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) != 0)
    {
        throw std::runtime_error(Mix_GetError());
    }
}

ch::audio::~audio()
{
    Mix_CloseAudio();
}
