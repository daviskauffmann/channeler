#include "world.h"

#include "map.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void world_load(struct world *world, const char *filename)
{
    printf("Loading world: %s\n", filename);

    world->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    struct json_object *maps_obj = json_object_object_get(root, "maps");
    world->num_maps = json_object_array_length(maps_obj);
    world->maps = malloc(world->num_maps * sizeof(*world->maps));
    for (size_t i = 0; i < world->num_maps; i++)
    {
        struct map *map = &world->maps[i];

        struct json_object *map_obj = json_object_array_get_idx(maps_obj, i);

        struct json_object *filename_obj = json_object_object_get(map_obj, "fileName");
        const char *assets_str = "assets/";
        const char *filename_str = json_object_get_string(filename_obj);
        const char *ext_str = ".json";
        size_t size = strlen(assets_str) + strlen(filename_str) + strlen(ext_str) + 1;
        char *map_filename = malloc(size);
        strcpy_s(map_filename, size, assets_str);
        strncat_s(map_filename, size, filename_str, strlen(filename_str) - strlen(".tmx"));
        strcat_s(map_filename, size, ext_str);
        map_load(map, map_filename);
    }
}

void world_unload(struct world *world)
{
    printf("Unloading world: %s\n", world->filename);

    for (size_t i = 0; i < world->num_maps; i++)
    {
        map_unload(&world->maps[i]);
    }
    free(world->maps);
}

size_t world_get_map_index(struct world *world, const char *filename)
{
    for (size_t i = 0; i < world->num_maps; i++)
    {
        if (strcmp(world->maps[i].filename, filename) == 0)
        {
            return i;
        }
    }
    return world->num_maps;
}
