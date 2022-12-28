#ifndef CH_SCENE_HPP
#define CH_SCENE_HPP

#include <SDL2/SDL.h>
#include <cstdint>
#include <memory>

namespace ch
{
    class display;

    class scene
    {
    public:
        scene(std::shared_ptr<ch::display> display)
            : display(display) {}
        virtual ~scene() = default;
        scene(const scene &other) = delete;
        scene &operator=(const scene &other) = delete;
        scene(scene &&other) = delete;
        scene &operator=(scene &&other) = delete;

        virtual ch::scene *handle_event(const SDL_Event &event) = 0;
        virtual ch::scene *update(
            float delta_time,
            const std::uint8_t *keys,
            std::uint32_t mouse,
            int mouse_x,
            int mouse_y) = 0;

    protected:
        std::shared_ptr<ch::display> display;

        template <typename T, typename... Args>
        ch::scene *change_scene(Args... args)
        {
            auto scene = new T(std::forward<Args>(args)...);

            delete this;

            return scene;
        }

        ch::scene *exit_game()
        {
            delete this;

            return nullptr;
        }
    };

}

#endif
