#ifndef DRAW_TEXT_HPP
#define DRAW_TEXT_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>

void draw_text(
    SDL_Renderer *renderer,
    TTF_Font *font,
    const std::size_t x,
    const std::size_t y,
    const std::size_t w,
    const SDL_Color &fg,
    const char *fmt, ...);

#endif
