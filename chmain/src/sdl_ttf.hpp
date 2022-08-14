#ifndef CH_SDL_TTF_HPP
#define CH_SDL_TTF_HPP

namespace ch
{
    class sdl_ttf
    {
    public:
        sdl_ttf();
        sdl_ttf(sdl_ttf &&other) = delete;
        sdl_ttf(const sdl_ttf &other) = delete;
        sdl_ttf &operator=(sdl_ttf &&other) = delete;
        sdl_ttf &operator=(const sdl_ttf &other) = delete;
        ~sdl_ttf();
    };
}

#endif
