#ifndef CH_SOUND_HPP
#define CH_SOUND_HPP

#include <SDL2/SDL_mixer.h>

namespace ch
{
    class sound
    {
    public:
        sound(const char *filename);
        ~sound();

        void play() const;

    private:
        Mix_Chunk *mix_chunk;
    };
}

#endif
