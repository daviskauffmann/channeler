#include "sound.hpp"

#include <stdexcept>

ch::sound::sound(const char *const filename)
{
    mix_chunk = Mix_LoadWAV(filename);
    if (!mix_chunk)
    {
        throw std::runtime_error(Mix_GetError());
    }
}

ch::sound::~sound()
{
    Mix_FreeChunk(mix_chunk);
}

void ch::sound::play() const
{
    Mix_PlayChannel(-1, mix_chunk, 0);
}
