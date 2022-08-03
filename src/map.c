#include "map.h"

#include "layer.h"
#include "tileset.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void map_load(struct map *map, char *filename)
{
    printf("Loading map: %s\n", filename);

    map->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    {
        struct json_object *width_obj = json_object_object_get(root, "width");
        map->width = json_object_get_int64(width_obj);

        struct json_object *height_obj = json_object_object_get(root, "height");
        map->height = json_object_get_int64(height_obj);
    }

    {
        map->num_layers = 0;
        map->layers = NULL;

        struct json_object *layers_obj = json_object_object_get(root, "layers");
        size_t num_layers = json_object_array_length(layers_obj);
        for (size_t i = 0; i < num_layers; i++)
        {
            struct json_object *layer_obj = json_object_array_get_idx(layers_obj, i);

            struct json_object *type_obj = json_object_object_get(layer_obj, "type");
            const char *type = json_object_get_string(type_obj);
            if (strcmp(type, "tilelayer") == 0)
            {
                size_t layer_index = map->num_layers++;
                map->layers = realloc(map->layers, map->num_layers * sizeof(*map->layers));
                map->layers[layer_index].tiles = malloc(map->width * map->height * sizeof(*map->layers[layer_index].tiles));

                struct json_object *data_obj = json_object_object_get(layer_obj, "data");
                for (size_t j = 0; j < map->width * map->height; j++)
                {
                    struct tile *tile = &map->layers[layer_index].tiles[j];

                    const uint32_t H_FLIP_FLAG = 0x80000000;
                    const uint32_t V_FLIP_FLAG = 0x40000000;
                    const uint32_t D_FLIP_FLAG = 0x20000000;

                    struct json_object *tile_obj = json_object_array_get_idx(data_obj, j);
                    int64_t gid = json_object_get_int64(tile_obj);
                    tile->gid = gid & ~(H_FLIP_FLAG | V_FLIP_FLAG | D_FLIP_FLAG);
                    tile->h_flip = gid & H_FLIP_FLAG;
                    tile->v_flip = gid & V_FLIP_FLAG;
                    tile->d_flip = gid & D_FLIP_FLAG;
                }
            }
            else if (strcmp(type, "objectgroup") == 0)
            {
                // TODO: object layer support in the map
                struct json_object *objects_obj = json_object_object_get(layer_obj, "objects");

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
            size_t size = strlen(assets_str) + strlen(source_str) + strlen(ext_str) + 1;
            char *tileset_filename = malloc(size);
            strcpy_s(tileset_filename, size, assets_str);
            strncat_s(tileset_filename, size, source_str, strlen(source_str) - strlen(".tsx"));
            strcat_s(tileset_filename, size, ext_str);
            tileset_load(tileset, tileset_filename);
        }
    }

    map->sprites = NULL;
}

void map_unload(struct map *map)
{
    printf("Unloading map: %s\n", map->filename);

    free(map->filename);

    for (size_t i = 0; i < map->num_layers; i++)
    {
        free(map->layers[i].tiles);
    }
    free(map->layers);
    map->layers = NULL;

    if (map->sprites)
    {
        for (size_t i = 0; i < map->num_tilesets; i++)
        {
            printf("Destroying texture: %s\n", map->tilesets[i].image);

            SDL_DestroyTexture(map->sprites[i]);
        }
        free(map->sprites);
        map->sprites = NULL;
    }

    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        tileset_unload(&map->tilesets[i]);
    }
    free(map->tilesets);
    map->tilesets = NULL;
}

void map_activate(struct map *map, SDL_Renderer *renderer)
{
    map->sprites = malloc(map->num_tilesets * sizeof(*map->sprites));
    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        printf("Loading texture: %s\n", map->tilesets[i].image);

        map->sprites[i] = IMG_LoadTexture(renderer, map->tilesets[i].image);
    }
}

void map_deactivate(struct map *map)
{
    for (size_t i = 0; i < map->num_tilesets; i++)
    {
        printf("Destroying texture: %s\n", map->tilesets[i].image);

        SDL_DestroyTexture(map->sprites[i]);
    }
    free(map->sprites);
    map->sprites = NULL;
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

bool map_is_solid(struct map *map, size_t x, size_t y)
{
    for (size_t i = 0; i < map->num_layers; i++)
    {
        struct layer *layer = &map->layers[i];
        struct tile *tile = layer_get_tile(layer, map, x, y);
        if (tile)
        {
            struct tileset *tileset = map_get_tileset(map, tile->gid);
            struct tile_data *tile_data = tileset_get_tile_data(tileset, tile->gid);
            if (tile_data->solid)
            {
                return true;
            }
        }
    }

    return false;
}
