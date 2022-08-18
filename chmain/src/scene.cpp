#include "scene.hpp"

ch::scene *ch::scene::current_scene = nullptr;

ch::scene *ch::scene::get_scene()
{
    return current_scene;
}

void ch::scene::delete_scene()
{
    delete current_scene;
    current_scene = nullptr;
}
