#include <ch/sdl.hpp>

#include <stdexcept>

ch::sdl::sdl(const std::uint32_t flags)
{
    if (SDL_Init(flags) != 0)
    {
        throw std::runtime_error(SDL_GetError());
    }
}

ch::sdl::~sdl()
{
    SDL_Quit();
}

std::uint64_t ch::sdl::get_ticks() const
{
    return SDL_GetTicks64();
}

const std::uint8_t *ch::sdl::get_keys() const
{
    return SDL_GetKeyboardState(nullptr);
}

ch::mouse_state ch::sdl::get_mouse_state() const
{
    ch::mouse_state state;
    state.mouse = SDL_GetMouseState(&state.mouse_x, &state.mouse_y);
    return state;
}

bool ch::sdl::poll_event(SDL_Event &event) const
{
    return SDL_PollEvent(&event);
}
