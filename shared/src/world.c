#include <shared/world.h>

#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void tileset_load(struct tileset *tileset, const char *filename)
{
    json_tokener *tok = json_tokener_new();
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *str = malloc(len);
    fread(str, 1, len, f);
    fclose(f);
    json_object *sprites = json_tokener_parse_ex(tok, str, len);
    json_tokener_free(tok);

    json_object *columns;
    json_object_object_get_ex(sprites, "columns", &columns);
    tileset->columns = json_object_get_int(columns);

    json_object *image;
    json_object_object_get_ex(sprites, "image", &image);
    tileset->image = json_object_get_string(image);
    tileset->image = "assets/1bitpack_kenney_1.2/tilesheet/colored-transparent_packed.png";

    json_object *tilecount;
    json_object_object_get_ex(sprites, "tilecount", &tilecount);
    tileset->tile_data = malloc(json_object_get_int(tilecount) * sizeof(tileset->tile_data[0]));

    json_object *tiles;
    json_object_object_get_ex(sprites, "tiles", &tiles);
    for (int j = 0; j < json_object_array_length(tiles); j++)
    {
        json_object *tile = json_object_array_get_idx(tiles, j);

        json_object *id;
        json_object_object_get_ex(tile, "id", &id);
        int tile_id = json_object_get_int(id);

        json_object *properties;
        json_object_object_get_ex(tile, "properties", &properties);
        for (int k = 0; k < json_object_array_length(properties); k++)
        {
            json_object *property = json_object_array_get_idx(properties, k);

            json_object *name;
            json_object_object_get_ex(property, "name", &name);
            const char *n = json_object_get_string(name);

            if (strcmp(n, "solid") == 0)
            {
                json_object *value;
                json_object_object_get_ex(property, "value", &value);
                tileset->tile_data[tile_id].solid = json_object_get_boolean(value);
            }
        }
    }
}

struct tile_data *tileset_get_tile_data(struct tileset *tileset, int gid)
{
    return &tileset->tile_data[gid - tileset->first_gid];
}

void world_load(struct world *world, const char *filename)
{
    json_tokener *tok = json_tokener_new();
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *str = malloc(len);
    fread(str, 1, len, f);
    fclose(f);
    json_object *map = json_tokener_parse_ex(tok, str, len);
    json_tokener_free(tok);

    json_object *width;
    json_object_object_get_ex(map, "width", &width);
    world->width = json_object_get_int(width);

    json_object *height_obj;
    json_object_object_get_ex(map, "height", &height_obj);
    world->height = json_object_get_int(height_obj);

    json_object *tilewidth;
    json_object_object_get_ex(map, "tilewidth", &tilewidth);
    world->tile_width = json_object_get_int(tilewidth);

    json_object *tileheight;
    json_object_object_get_ex(map, "tileheight", &tileheight);
    world->tile_height = json_object_get_int(tileheight);

    world->tiles = malloc(world->width * world->height * sizeof(world->tiles[0]));
    json_object *layers;
    json_object_object_get_ex(map, "layers", &layers);

    json_object *layer0 = json_object_array_get_idx(layers, 0);
    json_object *data;
    json_object_object_get_ex(layer0, "data", &data);
    for (int i = 0; i < world->width * world->height; i++)
    {
        json_object *tile = json_object_array_get_idx(data, i);
        long long gid = json_object_get_int64(tile);

        const unsigned H_FLIP_FLAG = 0x80000000;
        const unsigned V_FLIP_FLAG = 0x40000000;
        const unsigned D_FLIP_FLAG = 0x20000000;

        world->tiles[i].gid = gid & ~(H_FLIP_FLAG | V_FLIP_FLAG | D_FLIP_FLAG);
        world->tiles[i].h_flip = gid & H_FLIP_FLAG;
        world->tiles[i].v_flip = gid & V_FLIP_FLAG;
        world->tiles[i].d_flip = gid & D_FLIP_FLAG;
    }

    json_object *layer1 = json_object_array_get_idx(layers, 1);
    json_object *objects;
    json_object_object_get_ex(layer1, "objects", &objects);

    for (int i = 0; i < json_object_array_length(objects); i++)
    {
        world->mobs[i].alive = true;

        json_object *object = json_object_array_get_idx(objects, i);

        json_object *gid;
        json_object_object_get_ex(object, "gid", &gid);
        world->mobs[i].gid = json_object_get_int(gid);

        json_object *x;
        json_object_object_get_ex(object, "x", &x);
        world->mobs[i].x = (float)json_object_get_double(x);

        json_object *y;
        json_object_object_get_ex(object, "y", &y);
        world->mobs[i].y = (float)json_object_get_double(y);
    }

    json_object *tilesets;
    json_object_object_get_ex(map, "tilesets", &tilesets);
    world->tileset_count = json_object_array_length(tilesets);
    world->tilesets = malloc(world->tileset_count * sizeof(world->tilesets[0]));
    for (int i = 0; i < world->tileset_count; i++)
    {
        json_object *tileset = json_object_array_get_idx(tilesets, i);

        world->tilesets[i].index = i;

        json_object *firstgid;
        json_object_object_get_ex(tileset, "firstgid", &firstgid);
        world->tilesets[i].first_gid = json_object_get_int(firstgid);

        json_object *source;
        json_object_object_get_ex(tileset, "source", &source);
        const char *s = json_object_get_string(source);

        tileset_load(&world->tilesets[i], "assets/colored-transparent_packed.json"); // TODO: tileset.source = >.tsx->.json
    }
}

void world_update(struct world *world, float delta_time)
{
}

struct tile *world_get_tile(struct world *world, int x, int y)
{
    return &world->tiles[x + y * world->width];
}

struct tileset *world_get_tileset(struct world *world, int gid)
{
    int tileset_index = 0;
    for (int i = 0; i < world->tileset_count; i++)
    {
        if (world->tilesets[i].first_gid <= gid)
        {
            tileset_index = i;
        }
    }
    return &world->tilesets[tileset_index];
}
