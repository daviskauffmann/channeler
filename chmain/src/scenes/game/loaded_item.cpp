#include "loaded_item.hpp"

#include "../../sound.hpp"
#include "../../texture.hpp"
#include <ch/item.hpp>

ch::loaded_item::loaded_item(const ch::item &item, SDL_Renderer *const renderer)
{
    attack_sprite = std::make_unique<ch::texture>(renderer, item.attack_sprite_filename.c_str());
    attack_sound = std::make_unique<ch::sound>(item.attack_sound_filename.c_str());
}
