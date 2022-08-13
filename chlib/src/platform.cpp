#include <ch/platform.hpp>

#include <stdexcept>

ch::platform::platform(std::uint32_t flags)
{
    if (SDL_Init(flags) != 0)
    {
        throw std::runtime_error(SDL_GetError());
    }
}

ch::platform::~platform()
{
    SDL_Quit();
}

bool ch::platform::is_running() const
{
    return running;
}

void ch::platform::stop()
{
    running = false;
}

std::uint64_t ch::platform::get_ticks() const
{
    return SDL_GetTicks64();
}

float ch::platform::get_delta_time() const
{
    const auto current_time = get_ticks();
    const auto delta_time = (current_time - last_frame_time) / 1000.0f;
    last_frame_time = current_time;
    return delta_time;
}

const std::uint8_t *ch::platform::get_keys() const
{
    return SDL_GetKeyboardState(nullptr);
}

ch::mouse_state ch::platform::get_mouse_state() const
{
    ch::mouse_state state;
    state.mouse = SDL_GetMouseState(&state.mouse_x, &state.mouse_y);
    return state;
}

bool ch::platform::poll_event(SDL_Event &event)
{
    auto result = SDL_PollEvent(&event);

    switch (event.type)
    {
    case SDL_QUIT:
    {
        stop();
    }
    break;
    }

    return result;
}
