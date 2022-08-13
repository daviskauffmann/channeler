#include "scene.hpp"

ch::scene *ch::scene::current_scene = nullptr;

void ch::scene::delete_scene()
{
    if (current_scene)
    {
        delete current_scene;
        current_scene = nullptr;
    }
}
