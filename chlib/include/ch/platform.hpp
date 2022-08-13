#ifndef CH_PLATFORM_HPP
#define CH_PLATFORM_HPP

#include <SDL2/SDL.h>
#include <cstdint>

namespace ch
{
    struct mouse_state
    {
        std::uint32_t mouse;
        int mouse_x;
        int mouse_y;
    };

    class platform
    {
    public:
        platform(std::uint32_t flags);
        platform(platform &&other) = delete;
        platform(const platform &other) = delete;
        platform &operator=(platform &&other) = delete;
        platform &operator=(const platform &other) = delete;
        ~platform();

        bool is_running() const;
        void stop();

        std::uint64_t get_ticks() const;
        float get_delta_time() const;
        const std::uint8_t *get_keys() const;
        ch::mouse_state get_mouse_state() const;

        bool poll_event(SDL_Event &event);

    private:
        bool running = true;
        mutable std::uint64_t last_frame_time = 0;
    };
}

#endif
