#include "scene.hpp"
#include "scenes/menu_scene.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <enet/enet.h>
#include <spdlog/spdlog.h>

constexpr const char *window_title = "Channeler";
constexpr std::size_t window_width = 640;
constexpr std::size_t window_height = 480;

constexpr std::size_t fps_cap = 144;
constexpr std::size_t frame_delay = 1000 / fps_cap;

int main(int, char *[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());

        return 1;
    }
    atexit(SDL_Quit);

    constexpr int img_flags = IMG_INIT_PNG;
    if (IMG_Init(img_flags) != img_flags)
    {
        spdlog::error("Failed to initialize SDL_image: {}", IMG_GetError());

        return 1;
    }
    atexit(IMG_Quit);

    constexpr int mix_flags = 0;
    if (Mix_Init(mix_flags) != mix_flags)
    {
        spdlog::error("Failed to initialize SDL_mixer: {}", Mix_GetError());

        return 1;
    }
    atexit(Mix_Quit);

    if (TTF_Init() != 0)
    {
        spdlog::error("Failed to initialize SDL_ttf: {}", TTF_GetError());

        return 1;
    }
    atexit(TTF_Quit);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) != 0)
    {
        spdlog::error("Failed to initialize the mixer API: {}", Mix_GetError());

        return 1;
    }
    atexit(Mix_CloseAudio);

    if (enet_initialize() != 0)
    {
        spdlog::error("Failed to initialize ENet");

        return 1;
    }
    atexit(enet_deinitialize);

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(window_width, window_height, 0, &window, &renderer) != 0)
    {
        spdlog::error("Failed to create window and renderer: {}", SDL_GetError());

        return 1;
    }
    SDL_SetWindowTitle(window, window_title);

    ch::scene *scene = new ch::menu_scene(renderer);

    auto quit = false;
    while (!quit)
    {
        static std::uint32_t current_time = 0;
        const auto previous_time = current_time;
        current_time = SDL_GetTicks();
        const auto delta_time = (current_time - previous_time) / 1000.0f;

        const auto keys = SDL_GetKeyboardState(nullptr);

        int mouse_x, mouse_y;
        const auto mouse = SDL_GetMouseState(&mouse_x, &mouse_y);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_RETURN:
                {
                    if (keys[SDL_SCANCODE_LALT])
                    {
                        const auto flags = SDL_GetWindowFlags(window);
                        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                        {
                            SDL_SetWindowFullscreen(window, 0);
                        }
                        else
                        {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                    }
                }
                break;
                }
            }
            break;
            case SDL_QUIT:
            {
                quit = true;
            }
            break;
            }

            if (scene)
            {
                scene = scene->handle_event(event);
                if (!scene)
                {
                    quit = true;
                }
            }
        }

        SDL_RenderClear(renderer);

        if (scene)
        {
            scene = scene->update(keys, mouse, mouse_x, mouse_y, delta_time);
            if (!scene)
            {
                quit = true;
            }
        }

        SDL_RenderPresent(renderer);

        const auto frame_time = SDL_GetTicks() - current_time;
        if (frame_delay > frame_time)
        {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    if (scene)
    {
        delete scene;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
