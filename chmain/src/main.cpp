#include "audio.hpp"
#include "display.hpp"
#include "image.hpp"
#include "mixer.hpp"
#include "scene.hpp"
#include "scenes/menu_scene.hpp"
#include "ttf.hpp"
#include <ch/enet.hpp>
#include <ch/platform.hpp>

int main(int, char *[])
{
    ch::platform platform(SDL_INIT_AUDIO | SDL_INIT_VIDEO);

    const ch::image image;
    const ch::mixer mixer;
    const ch::ttf ttf;
    const ch::audio audio;
    const ch::enet enet;

    const ch::display display(640, 480);
    display.set_title("Channeler");
    display.set_vsync(true);

    ch::scene::change_scene<ch::menu_scene>(std::ref(display));

    while (platform.is_running())
    {
        const auto delta_time = platform.get_delta_time();
        const auto keys = platform.get_keys();
        const auto [mouse, mouse_x, mouse_y] = platform.get_mouse_state();

        SDL_Event event;
        while (platform.poll_event(event))
        {
            display.handle_event(event);

            if (ch::scene::current_scene)
            {
                ch::scene::current_scene->handle_event(event);
                if (!ch::scene::current_scene)
                {
                    platform.stop();
                }
            }
        }

        display.clear();

        if (ch::scene::current_scene)
        {
            ch::scene::current_scene->update(delta_time, keys, mouse, mouse_x, mouse_y);
            if (!ch::scene::current_scene)
            {
                platform.stop();
            }
        }

        display.present();
    }

    if (ch::scene::current_scene)
    {
        ch::scene::delete_scene();
    }

    return 0;
}
