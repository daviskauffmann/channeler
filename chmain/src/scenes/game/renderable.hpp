#ifndef CH_SCENES_RENDERABLE_HPP
#define CH_SCENES_RENDERABLE_HPP

#include <SDL2/SDL.h>

namespace ch
{
    class texture;

    struct renderable
    {
        ch::texture *texture;
        SDL_Rect srcrect;
        SDL_Rect dstrect;
        double angle;

        int y;
    };
}

#endif
