#include <shared/world.h>

#include <json-c/json.h>
#include <shared/map.h>
#include <stdio.h>
#include <string.h>

void world_load(struct world *world, const char *filename, bool load_maps)
{
    world->filename = filename;

    json_object *root = json_object_from_file(filename);

    json_object *maps;
    json_object_object_get_ex(root, "maps", &maps);
    world->num_maps = json_object_array_length(maps);

    world->maps = malloc(world->num_maps * sizeof(world->maps[0]));
    for (int i = 0; i < world->num_maps; i++)
    {
        json_object *map = json_object_array_get_idx(maps, i);

        json_object *fileName;
        json_object_object_get_ex(map, "fileName", &fileName);
        const char *fileName_value = json_object_get_string(fileName);
        const char *assets = "assets/";
        const char *ext = ".json";
        char *map_filename = malloc(strlen(assets) + strlen(fileName_value) + strlen(ext) + 1);
        strcpy(map_filename, assets);
        strncat(map_filename, fileName_value, strlen(fileName_value) - strlen(".tmx"));
        strcat(map_filename, ext);
        map_init(&world->maps[i], map_filename);

        if (load_maps)
        {
            map_load(&world->maps[i]);
        }
    }

    printf("World loaded: %s\n", world->filename);
}

void world_unload(struct world *world, bool unload_maps)
{
    for (int i = 0; i < world->num_maps; i++)
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
