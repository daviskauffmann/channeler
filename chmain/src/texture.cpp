#include "texture.hpp"

#include <SDL2/SDL_image.h>
#include <stdexcept>

ch::texture::texture(SDL_Renderer *renderer, const char *file)
{
    sdl_texture = IMG_LoadTexture(renderer, file);
    if (!sdl_texture)
    {
        throw std::runtime_error(IMG_GetError());
    }
}

ch::texture::~texture()
{
    SDL_DestroyTexture(sdl_texture);
}

void ch::texture::render(
    SDL_Renderer *const renderer,
    const SDL_Rect *const srcrect,
    const SDL_Rect *const dstrect) const
{
    SDL_RenderCopy(renderer, sdl_texture, srcrect, dstrect);
}

void ch::texture::render_ex(
    SDL_Renderer *const renderer,
    const SDL_Rect *const srcrect,
    const SDL_Rect *const dstrect,
    const double angle,
    const SDL_Point *const center,
    const SDL_RendererFlip flip) const
{
    SDL_RenderCopyEx(renderer, sdl_texture, srcrect, dstrect, angle, center, flip);
}
