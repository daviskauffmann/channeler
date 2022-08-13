#ifndef CH_TEXTURE_HPP
#define CH_TEXTURE_HPP

#include <SDL2/SDL.h>

namespace ch
{
    class texture
    {
    public:
        texture(SDL_Renderer *renderer, const char *file);
        ~texture();

        void render(
            SDL_Renderer *renderer,
            const SDL_Rect *srcrect,
            const SDL_Rect *dstrect) const;

        void render_ex(
            SDL_Renderer *renderer,
            const SDL_Rect *srcrect,
            const SDL_Rect *dstrect,
            const double angle,
            const SDL_Point *center,
            const SDL_RendererFlip flip) const;

    private:
        SDL_Texture *sdl_texture;
    };
}

#endif
