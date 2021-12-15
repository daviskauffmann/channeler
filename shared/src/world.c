#include <shared/world.h>

#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void world_load(struct world *world, const char *tiles_filename, const char *sprites_filename)
{
    srand((unsigned int)time(NULL));

    {
        json_tokener *tok = json_tokener_new();
        FILE *f = fopen(tiles_filename, "rb");
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *str = malloc(len);
        fread(str, 1, len, f);
        fclose(f);
        json_object *tiles = json_tokener_parse_ex(tok, str, len);
        json_tokener_free(tok);

        json_object *width;
        json_object_object_get_ex(tiles, "width", &width);
        world->width = json_object_get_int(width);

        json_object *height_obj;
        json_object_object_get_ex(tiles, "height", &height_obj);
        world->height = json_object_get_int(height_obj);

        json_object *tilewidth;
        json_object_object_get_ex(tiles, "tilewidth", &tilewidth);
        world->tile_width = json_object_get_int(tilewidth);

        json_object *tileheight;
        json_object_object_get_ex(tiles, "tileheight", &tileheight);
        world->tile_height = json_object_get_int(tileheight);

        world->tiles = malloc(world->width * world->height * sizeof(world->tiles[0]));
        json_object *layers;
        json_object_object_get_ex(tiles, "layers", &layers);
        json_object *layer = json_object_array_get_idx(layers, 0);
        json_object *data;
        json_object_object_get_ex(layer, "data", &data);
        for (int i = 0; i < world->width * world->height; i++)
        {
            json_object *tile = json_object_array_get_idx(data, i);
            world->tiles[i] = json_object_get_int(tile) - 1;
        }
    }

    {
        json_tokener *tok = json_tokener_new();
        FILE *f = fopen(sprites_filename, "rb");
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *str = malloc(len);
        fread(str, 1, len, f);
        fclose(f);
        json_object *sprites = json_tokener_parse_ex(tok, str, len);
        json_tokener_free(tok);

        json_object *tilecount;
        json_object_object_get_ex(sprites, "tilecount", &tilecount);
        int tile_count = json_object_get_int(tilecount);

        world->tile_data = malloc(tile_count * sizeof(world->tile_data[0]));
        json_object *tiles;
        json_object_object_get_ex(sprites, "tiles", &tiles);
        for (int i = 0; i < json_object_array_length(tiles); i++)
        {
            json_object *tile = json_object_array_get_idx(tiles, i);

            json_object *id;
            json_object_object_get_ex(tile, "id", &id);
            int tile_id = json_object_get_int(id);

            json_object *properties;
            json_object_object_get_ex(tile, "properties", &properties);
            for (int j = 0; j < json_object_array_length(properties); j++)
            {
                json_object *property = json_object_array_get_idx(properties, j);

                json_object *name;
                json_object_object_get_ex(property, "name", &name);
                const char *n = json_object_get_string(name);

                if (strcmp(n, "solid") == 0)
                {
                    json_object *value;
                    json_object_object_get_ex(property, "value", &value);
                    world->tile_data[tile_id].solid = json_object_get_boolean(value);
                }
            }
        }
    }

    for (int i = 0; i < NUM_MOBS; i++)
    {
        world->mobs[i].alive = true;
        world->mobs[i].x = (float)(rand() % 1000);
        world->mobs[i].y = (float)(rand() % 500);
    }
}

void world_update(struct world *world, float delta_time)
{
    // static float timer = 0;
    // timer += delta_time;
    // if (timer >= 2.0f)
    // {
    //     timer = 0;

    //     for (int i = 0; i < NUM_MOBS; i++)
    //     {
    //         world->mobs[i].x = rand() % 1000;
    //         world->mobs[i].y = rand() % 500;
    //     }
    // }
}
