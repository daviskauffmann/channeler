#ifndef CH_SCENES_LOADED_ITEM_HPP
#define CH_SCENES_LOADED_ITEM_HPP

#include <SDL2/SDL.h>
#include <memory>

namespace ch
{
    struct item;
    class texture;
    class sound;

    struct loaded_item
    {
        std::unique_ptr<ch::texture> attack_sprite;
        std::unique_ptr<ch::sound> attack_sound;

        loaded_item(const ch::item &item, SDL_Renderer *renderer);
    };
}

#endif
