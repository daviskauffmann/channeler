#include <shared/map.h>

#include <json-c/json.h>
#include <shared/tileset.h>
#include <stdio.h>
#include <string.h>

void map_init(struct map *map, char *filename)
{
    map->filename = filename;

    printf("Map initialized: %s\n", map->filename);
}

void map_uninit(struct map *map)
{
    printf("Map uninitialized: %s\n", map->filename);
    free(map->filename);
}

void map_load(struct map *map)
{
    struct json_object *root = json_object_from_file(map->filename);

    {
        struct json_object *width_obj = json_object_object_get(root, "width");
        map->width = json_object_get_int64(width_obj);

        struct json_object *height_obj = json_object_object_get(root, "height");
        map->height = json_object_get_int64(height_obj);

        map->tiles = malloc(map->width * map->height * sizeof(*map->tiles));
    }

    {
        struct json_object *layers_obj = json_object_object_get(root, "layers");

        {
            struct json_object *layer_0_obj = json_object_array_get_idx(layers_obj, 0);
            struct json_object *data_obj = json_object_object_get(layer_0_obj, "data");
            for (size_t i = 0; i < map->width * map->height; i++)
            {
                struct tile *tile = &map->tiles[i];

                const unsigned H_FLIP_FLAG = 0x80000000;
                const unsigned V_FLIP_FLAG = 0x40000000;
                const unsigned D_FLIP_FLAG = 0x20000000;

                struct json_object *tile_obj = json_object_array_get_idx(data_obj, i);
                int64_t gid = json_object_get_int64(tile_obj);
                tile->gid = gid & ~(H_FLIP_FLAG | V_FLIP_FLAG | D_FLIP_FLAG);
                tile->h_flip = gid & H_FLIP_FLAG;
                tile->v_flip = gid & V_FLIP_FLAG;
                tile->d_flip = gid & D_FLIP_FLAG;
            }
        }

        {
            struct json_object *layer_1_obj = json_object_array_get_idx(layers_obj, 1);
            struct json_object *objects_obj = json_object_object_get(layer_1_obj, "objects");

            for (size_t i = 0; i < MAX_MOBS; i++)
            {
                struct mob *mob = &map->mobs[i];
                mob->gid = 0;
            }

            for (size_t i = 0; i < json_object_array_length(objects_obj); i++)
            {
                struct mob *mob = &map->mobs[i];

                // TODO: check object type, currently all objects are treated as mobs
                struct json_object *object_obj = json_object_array_get_idx(objects_obj, i);

                struct json_object *gid_obj = json_object_object_get(object_obj, "gid");
                mob->gid = json_object_get_int(gid_obj);

                struct json_object *x_obj = json_object_object_get(object_obj, "x");
                mob->origin_x = mob->x = (float)json_object_get_double(x_obj);

                struct json_object *y_obj = json_object_object_get(object_obj, "y");
                mob->origin_y = mob->y = (float)json_object_get_double(y_obj);

                mob->alive = true;
                mob->respawn_timer = 0;
            }
        }
    }

    {
        struct json_object *tile_width_obj = json_object_object_get(root, "tilewidth");
        map->tile_width = json_object_get_int64(tile_width_obj);

        struct json_object *tile_height_obj = json_object_object_get(root, "tileheight");
        map->tile_height = json_object_get_int64(tile_height_obj);
    }

    {
        struct json_object *tilesets_obj = json_object_object_get(root, "tilesets");
        map->num_tilesets = json_object_array_length(tilesets_obj);
        map->tilesets = malloc(map->num_tilesets * sizeof(*map->tilesets));
        for (size_t i = 0; i < map->num_tilesets; i++)
        {
            struct tileset *tileset = &map->tilesets[i];
            tileset->index = i;

            struct json_object *tileset_obj = json_object_array_get_idx(tilesets_obj, i);

            struct json_object *first_gid_obj = json_object_object_get(tileset_obj, "firstgid");
            tileset->first_gid = json_object_get_int(first_gid_obj);

            struct json_object *source_obj = json_object_object_get(tileset_obj, "source");
            const char *assets_str = "assets/";
            const char *source_str = json_object_get_string(source_obj);
            const char *ext_str = ".json";
            char *tileset_filename = malloc(strlen(assets_str) + strlen(source_str) + strlen(ext_str) + 1);
            strcpy(tileset_filename, assets_str);
            strncat(tileset_filename, source_str, strlen(source_str) - strlen(".tsx"));
            strcat(tileset_filename, ext_str);
            tileset_load(tileset, tileset_filename);
        }
    }

    printf("Map loaded: %s\n", map->filename);
}

void map_unload(struct map *map)
{
    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        tileset_unload(&map->tilesets[i]);
    }
    free(map->tilesets);
    map->tilesets = NULL;

    free(map->tiles);
    map->tiles = NULL;

    printf("Map unloaded: %s\n", map->filename);
}

void map_update(struct map *map, float delta_time)
{
    for (size_t i = 0; i < MAX_MOBS; i++)
    {
        struct mob *mob = &map->mobs[i];
        if (mob->gid != 0 && !mob->alive)
        {
            mob->respawn_timer -= delta_time;
            if (mob->respawn_timer <= 0)
            {
                mob->x = mob->origin_x;
                mob->y = mob->origin_y;
                mob->alive = true;
            }
        }
    }
}

struct tile *map_get_tile(struct map *map, size_t x, size_t y)
{
    if (x < map->width && y < map->height)
    {
        return &map->tiles[x + y * map->width];
    }
    return NULL;
}

struct tileset *map_get_tileset(struct map *map, size_t gid)
{
    size_t tileset_index = 0;
    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        if (map->tilesets[i].first_gid <= gid)
        {
            tileset_index = i;
        }
    }
    return &map->tilesets[tileset_index];
}
