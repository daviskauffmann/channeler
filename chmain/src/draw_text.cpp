#include "draw_text.hpp"

#include <stdarg.h>

void draw_text(
    SDL_Renderer *renderer,
    TTF_Font *font,
    const std::size_t x,
    const std::size_t y,
    const std::size_t w,
    SDL_Color fg,
    const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto size = vsnprintf(nullptr, 0, fmt, args) + 1;
    auto text = static_cast<char *>(malloc(size));
    vsprintf_s(text, size, fmt, args);
    va_end(args);

    auto surface = TTF_RenderText_Blended_Wrapped(font, text, fg, static_cast<uint32_t>(w));
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect dstrect = {
        static_cast<int>(x),
        static_cast<int>(y),
        surface->w,
        surface->h};

    SDL_RenderCopy(renderer, texture, nullptr, &dstrect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    free(text);
}
