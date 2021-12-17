#include <shared/tileset.h>

#include <json-c/json.h>
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
    json_object *root = json_tokener_parse_ex(tok, str, len);
    free(str);
    json_tokener_free(tok);

    json_object *columns;
    json_object_object_get_ex(root, "columns", &columns);
    tileset->columns = json_object_get_int(columns);

    json_object *image;
    json_object_object_get_ex(root, "image", &image);
    tileset->image = json_object_get_string(image);
    tileset->image = "assets/1bitpack_kenney_1.2/tilesheet/colored-transparent_packed.png";

    json_object *tilecount;
    json_object_object_get_ex(root, "tilecount", &tilecount);
    tileset->tile_data = malloc(json_object_get_int(tilecount) * sizeof(tileset->tile_data[0]));

    json_object *tiles;
    json_object_object_get_ex(root, "tiles", &tiles);
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

void tileset_delete(struct tileset *tileset)
{
    free(tileset->tile_data);
}

struct tile_data *tileset_get_tile_data(struct tileset *tileset, int gid)
{
    return &tileset->tile_data[gid - tileset->first_gid];
}
