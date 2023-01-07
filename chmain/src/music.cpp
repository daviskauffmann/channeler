#include "music.hpp"

#include <stdexcept>

ch::music::music(const char *const filename)
{
    mix_music = Mix_LoadMUS(filename);
    if (!mix_music)
    {
        throw std::runtime_error(Mix_GetError());
    }

    Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
}

ch::music::~music()
{
    Mix_FreeMusic(mix_music);
}

void ch::music::play() const
{
    Mix_PlayMusic(mix_music, -1);
}

void ch::music::halt() const
{
    Mix_HaltMusic();
}
