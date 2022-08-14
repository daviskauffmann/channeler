#ifndef CH_SDL_MIXER_HPP
#define CH_SDL_MIXER_HPP

namespace ch
{
    class sdl_mixer
    {
    public:
        sdl_mixer();
        sdl_mixer(sdl_mixer &&other) = delete;
        sdl_mixer(const sdl_mixer &other) = delete;
        sdl_mixer &operator=(sdl_mixer &&other) = delete;
        sdl_mixer &operator=(const sdl_mixer &other) = delete;
        ~sdl_mixer();
    };
}

#endif
