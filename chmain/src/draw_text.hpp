#ifndef DRAW_TEXT_HPP
#define DRAW_TEXT_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>
#include <fmt/format.h>
#include <string>

template <typename... Args>
auto draw_text(
    SDL_Renderer *const renderer,
    TTF_Font *const font,
    const std::size_t x,
    const std::size_t y,
    const std::size_t w,
    const SDL_Color &fg,
    const std::string &fmt,
    Args... args)
{
    const std::string text = fmt::format(fmt, std::forward<Args>(args)...);

    const auto surface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), fg, static_cast<uint32_t>(w));
    const auto texture = SDL_CreateTextureFromSurface(renderer, surface);

    const SDL_Rect dstrect = {
        static_cast<int>(x),
        static_cast<int>(y),
        surface->w,
        surface->h};

    SDL_RenderCopy(renderer, texture, nullptr, &dstrect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

#endif
