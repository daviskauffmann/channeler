#include "audio.hpp"
#include "display.hpp"
#include "scene.hpp"
#include "scenes/menu/menu.hpp"
#include "sdl_image.hpp"
#include "sdl_mixer.hpp"
#include "sdl_ttf.hpp"
#include <ch/enet.hpp>
#include <ch/sdl.hpp>
#include <memory>

int main(int, char *[])
{
    const ch::sdl sdl(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    const ch::sdl_image sdl_image;
    const ch::sdl_mixer sdl_mixer;
    const ch::audio audio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
    const ch::sdl_ttf sdl_ttf;
    const ch::enet enet;

    const auto display = std::make_shared<ch::display>(640, 480);
    display->set_title("Channeler");
    display->set_vsync(true);

    ch::scene *scene = new ch::menu(display);

    std::uint64_t current_time = 0;
    bool running = true;
    while (running)
    {
        const auto previous_time = current_time;
        current_time = sdl.get_ticks();
        const auto delta_time = (current_time - previous_time) / 1000.0f;

        const auto keys = sdl.get_keys();
        const auto [mouse, mouse_x, mouse_y] = sdl.get_mouse_state();

        SDL_Event event;
        while (sdl.poll_event(event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
            {
                running = false;
            }
            break;
            }

            display->handle_event(event);

            if (scene)
            {
                scene = scene->handle_event(event);
                if (!scene)
                {
                    running = false;
                }
            }
        }

        display->clear();

        if (scene)
        {
            scene = scene->update(delta_time, keys, mouse, mouse_x, mouse_y);
            if (!scene)
            {
                running = false;
            }
        }

        display->present();
    }

    if (scene)
    {
        delete scene;
    }

    return 0;
}
