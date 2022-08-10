#ifndef SCENE_HPP
#define SCENE_HPP

#include <SDL2/SDL.h>
#include <cstdint>

namespace ch
{
    class scene
    {
    public:
        scene(SDL_Renderer *renderer)
            : renderer(renderer) {}
        virtual ~scene() {}

        virtual ch::scene *handle_event(const SDL_Event &event) = 0;
        virtual ch::scene *update(
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y,
            float delta_time) = 0;

    protected:
        SDL_Renderer *renderer;
    };

}

#endif
