#include <shared/world.h>

#include <json-c/json.h>
#include <shared/map.h>
#include <stdio.h>
#include <string.h>

void world_load(struct world *world, const char *filename, bool load_maps)
{
    json_object *root = json_object_from_file(filename);

    json_object *maps;
    json_object_object_get_ex(root, "maps", &maps);
    world->num_maps = json_object_array_length(maps);

    world->maps = malloc(world->num_maps * sizeof(world->maps[0]));
    for (int i = 0; i < world->num_maps; i++)
    {
        struct map *map = &world->maps[i];
        if (load_maps)
        {
            map_load(map, i == 0 ? "assets/map1.json" : "assets/map2.json");
        }
        else
        {
            map->filename = i == 0 ? "assets/map1.json" : "assets/map2.json";
            map->tiles = NULL;
            map->tilesets = NULL;
        }
    }
}

void world_unload(struct world *world)
{
    for (int i = 0; i < world->num_maps; i++)
    {
        map_unload(&world->maps[i]);
    }
    free(world->maps);
}

int world_get_map_index(struct world *world, const char *filename)
{
    for (int i = 0; i < world->num_maps; i++)
    {
        if (strcmp(world->maps[i].filename, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}
