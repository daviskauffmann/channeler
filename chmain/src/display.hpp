#ifndef CH_DISPLAY_HPP
#define CH_DISPLAY_HPP

#include <SDL2/SDL.h>
#include <cstddef>

namespace ch
{
    class display
    {
    public:
        display(int width, int height);
        ~display();
        display(const display &other) = delete;
        display &operator=(const display &other) = delete;
        display(display &&other) = delete;
        display &operator=(display &&other) = delete;

        std::size_t get_width() const;
        std::size_t get_height() const;
        SDL_Renderer *get_renderer() const;

        void set_title(const char *title) const;
        void set_vsync(bool enabled) const;
        void toggle_fullscreen() const;

        void handle_event(const SDL_Event &event) const;

        void clear() const;
        void present() const;

    private:
        std::size_t width;
        std::size_t height;
        SDL_Window *window;
        SDL_Renderer *renderer;
    };
}

#endif
