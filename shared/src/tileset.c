#include <shared/tileset.h>

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void tileset_load(struct tileset *tileset, char *filename)
{
    tileset->filename = filename;

    json_object *root = json_object_from_file(filename);

    json_object *columns;
    json_object_object_get_ex(root, "columns", &columns);
    tileset->columns = json_object_get_int(columns);

    json_object *image;
    json_object_object_get_ex(root, "image", &image);
    const char *image_value = json_object_get_string(image);
    const char *assets = "assets/";
    tileset->image = malloc(strlen(assets) + strlen(image_value) + 1);
    strcpy(tileset->image, assets);
    strcat(tileset->image, image_value);

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

    printf("Tileset loaded: %s\n", tileset->filename);
}

void tileset_unload(struct tileset *tileset)
{
    free(tileset->tile_data);

    printf("Tileset unloaded: %s\n", tileset->filename);
    free(tileset->filename);
}

struct tile_data *tileset_get_tile_data(struct tileset *tileset, int gid)
{
    return &tileset->tile_data[gid - tileset->first_gid];
}
