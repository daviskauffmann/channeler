#ifndef CH_SCENE_HPP
#define CH_SCENE_HPP

#include <SDL2/SDL.h>
#include <cstdint>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace ch
{
    class display;

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
            catch (const std::exception &e)
            {
                spdlog::error("Failed to change scene: {}", e.what());
            }
        }

        static void delete_scene();

        scene(std::shared_ptr<ch::display> display)
            : display(display) {}
        scene(scene &&other) = delete;
        scene(const scene &other) = delete;
        scene &operator=(scene &&other) = delete;
        scene &operator=(const scene &other) = delete;
        virtual ~scene() = default;

        virtual void handle_event(const SDL_Event &event) = 0;
        virtual void update(
            float delta_time,
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y) = 0;

    protected:
        std::shared_ptr<ch::display> display;
    };

}

#endif
