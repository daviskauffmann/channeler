#ifndef SCENE_HPP
#define SCENE_HPP

#include <SDL2/SDL.h>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace ch
{
    class scene
    {
    public:
        static scene *current_scene;

        template <typename T, typename... Args>
        static void change_scene(Args... args)
        {
            try
            {
                const auto new_scene = new T(std::forward<Args>(args)...);
                delete current_scene;
                current_scene = new_scene;
            }
            catch (const std::runtime_error &e)
            {
                spdlog::error("Failed to change scene: {}", e.what());
            }
        }

        static void delete_scene();

        scene(SDL_Renderer *renderer)
            : renderer(renderer) {}
        scene(scene &&other) = delete;
        scene(const scene &other) = delete;
        scene &operator=(scene &&other) = delete;
        scene &operator=(const scene &other) = delete;
        virtual ~scene() = default;

        virtual void handle_event(const SDL_Event &event) = 0;
        virtual void update(
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
