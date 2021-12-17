#include <shared/map.h>

#include <json-c/json.h>
#include <shared/tileset.h>
#include <stdio.h>
#include <string.h>

void map_load(struct map *map, const char *filename)
{
    json_tokener *tok = json_tokener_new();
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *str = malloc(len);
    fread(str, 1, len, f);
    fclose(f);
    json_object *root = json_tokener_parse_ex(tok, str, len);
    free(str);
    json_tokener_free(tok);

    json_object *width;
    json_object_object_get_ex(root, "width", &width);
    map->width = json_object_get_int(width);

    json_object *height_obj;
    json_object_object_get_ex(root, "height", &height_obj);
    map->height = json_object_get_int(height_obj);

    json_object *tilewidth;
    json_object_object_get_ex(root, "tilewidth", &tilewidth);
    map->tile_width = json_object_get_int(tilewidth);

    json_object *tileheight;
    json_object_object_get_ex(root, "tileheight", &tileheight);
    map->tile_height = json_object_get_int(tileheight);

    map->tiles = malloc(map->width * map->height * sizeof(map->tiles[0]));
    json_object *layers;
    json_object_object_get_ex(root, "layers", &layers);

    json_object *layer0 = json_object_array_get_idx(layers, 0);
    json_object *data;
    json_object_object_get_ex(layer0, "data", &data);
    for (int i = 0; i < map->width * map->height; i++)
    {
        json_object *tile = json_object_array_get_idx(data, i);
        long long gid = json_object_get_int64(tile);

        const unsigned H_FLIP_FLAG = 0x80000000;
        const unsigned V_FLIP_FLAG = 0x40000000;
        const unsigned D_FLIP_FLAG = 0x20000000;

        map->tiles[i].gid = gid & ~(H_FLIP_FLAG | V_FLIP_FLAG | D_FLIP_FLAG);
        map->tiles[i].h_flip = gid & H_FLIP_FLAG;
        map->tiles[i].v_flip = gid & V_FLIP_FLAG;
        map->tiles[i].d_flip = gid & D_FLIP_FLAG;
    }

    json_object *layer1 = json_object_array_get_idx(layers, 1);
    json_object *objects;
    json_object_object_get_ex(layer1, "objects", &objects);

    for (int i = 0; i < json_object_array_length(objects); i++)
    {
        // TODO: check object type, currently all objects are treated as mobs
        json_object *object = json_object_array_get_idx(objects, i);

        struct mob *mob = &map->mobs[i];

        json_object *gid;
        json_object_object_get_ex(object, "gid", &gid);
        mob->gid = json_object_get_int(gid);

        json_object *x;
        json_object_object_get_ex(object, "x", &x);
        mob->origin_x = mob->x = (float)json_object_get_double(x);

        json_object *y;
        json_object_object_get_ex(object, "y", &y);
        mob->origin_y = mob->y = (float)json_object_get_double(y);

        mob->alive = true;
        mob->respawn_timer = 0;
    }

    json_object *tilesets;
    json_object_object_get_ex(root, "tilesets", &tilesets);
    map->tileset_count = json_object_array_length(tilesets);
    map->tilesets = malloc(map->tileset_count * sizeof(map->tilesets[0]));
    for (int i = 0; i < map->tileset_count; i++)
    {
        json_object *tileset = json_object_array_get_idx(tilesets, i);

        map->tilesets[i].index = i;

        json_object *firstgid;
        json_object_object_get_ex(tileset, "firstgid", &firstgid);
        map->tilesets[i].first_gid = json_object_get_int(firstgid);

        json_object *source;
        json_object_object_get_ex(tileset, "source", &source);
        const char *s = json_object_get_string(source);

        tileset_load(&map->tilesets[i], "assets/colored-transparent_packed.json"); // TODO: tileset.source = >.tsx->.json
    }
}

void map_unload(struct map *map)
{
    free(map->tiles);

    for (int i = 0; i < map->tileset_count; i++)
    {
        tileset_unload(&map->tilesets[i]);
    }
    free(map->tilesets);
}

void map_update(struct map *map, float delta_time)
{
    for (int i = 0; i < MAX_MOBS; i++)
    {
        struct mob *mob = &map->mobs[i];
        if (!mob->alive)
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

struct tile *map_get_tile(struct map *map, int x, int y)
{
    return &map->tiles[x + y * map->width];
}

struct tileset *map_get_tileset(struct map *map, int gid)
{
    int tileset_index = 0;
    for (int i = 0; i < map->tileset_count; i++)
    {
        if (map->tilesets[i].first_gid <= gid)
        {
            tileset_index = i;
        }
    }
    return &map->tilesets[tileset_index];
}
