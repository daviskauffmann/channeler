#ifndef CH_AUDIO_HPP
#define CH_AUDIO_HPP

#include <SDL2/SDL_mixer.h>

namespace ch
{
    class audio
    {
    public:
        audio(int frequency, Uint16 format, int channels, int chunk_size);
        ~audio();
        audio(const audio &other) = delete;
        audio &operator=(const audio &other) = delete;
        audio(audio &&other) = delete;
        audio &operator=(audio &&other) = delete;
    };
}

#endif
