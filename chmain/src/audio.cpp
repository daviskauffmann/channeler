#include "audio.hpp"

#include <stdexcept>

ch::audio::audio(
    const int frequency,
    const Uint16 format,
    const int channels,
    const int chunk_size)
{
    if (Mix_OpenAudio(frequency, format, channels, chunk_size) != 0)
    {
        throw std::runtime_error(Mix_GetError());
    }
}

ch::audio::~audio()
{
    Mix_CloseAudio();
}
