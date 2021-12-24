#include <shared/world.h>

#include <json-c/json.h>
#include <shared/map.h>
#include <stdio.h>
#include <string.h>

void world_load(struct world *world, const char *filename, bool load_maps)
{
    world->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    struct json_object *maps_obj = json_object_object_get(root, "maps");
    world->num_maps = json_object_array_length(maps_obj);
    world->maps = malloc(world->num_maps * sizeof(world->maps[0]));
    for (size_t i = 0; i < world->num_maps; i++)
    {
        struct map *map = &world->maps[i];

        struct json_object *map_obj = json_object_array_get_idx(maps_obj, i);

        struct json_object *filename_obj = json_object_object_get(map_obj, "fileName");
        const char *assets_str = "assets/";
        const char *filename_str = json_object_get_string(filename_obj);
        const char *ext_str = ".json";
        char *map_filename = malloc(strlen(assets_str) + strlen(filename_str) + strlen(ext_str) + 1);
        strcpy(map_filename, assets_str);
        strncat(map_filename, filename_str, strlen(filename_str) - strlen(".tmx"));
        strcat(map_filename, ext_str);
        map_init(map, map_filename);

        if (load_maps)
        {
            map_load(map);
        }
    }

    printf("World loaded: %s\n", world->filename);
}

void world_unload(struct world *world, bool unload_maps)
{
    for (size_t i = 0; i < world->num_maps; i++)
    {
        if (unload_maps)
        {
            map_unload(&world->maps[i]);
        }

        map_uninit(&world->maps[i]);
    }
    free(world->maps);

    printf("World unloaded: %s\n", world->filename);
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
    return -1;
}
