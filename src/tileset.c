#include "tileset.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

void tileset_load(struct tileset *tileset, char *filename)
{
    printf("Loading tileset: %s\n", filename);

    tileset->filename = filename;

    struct json_object *root = json_object_from_file(filename);

    struct json_object *columns_obj = json_object_object_get(root, "columns");
    tileset->columns = json_object_get_int64(columns_obj);

    struct json_object *image_obj = json_object_object_get(root, "image");
    const char *assets_str = "assets/";
    const char *image_str = json_object_get_string(image_obj);
    size_t size = strlen(assets_str) + strlen(image_str) + 1;
    tileset->image = malloc(size);
    strcpy_s(tileset->image, size, assets_str);
    strcat_s(tileset->image, size, image_str);

    struct json_object *tile_count_obj = json_object_object_get(root, "tilecount");
    tileset->tile_data = malloc(json_object_get_int(tile_count_obj) * sizeof(*tileset->tile_data));

    struct json_object *tiles_obj = json_object_object_get(root, "tiles");
    for (size_t i = 0; i < json_object_array_length(tiles_obj); i++)
    {
        struct json_object *tile_obj = json_object_array_get_idx(tiles_obj, i);

        struct json_object *id_obj = json_object_object_get(tile_obj, "id");
        int64_t id = json_object_get_int64(id_obj);

        struct tile_data *tile_data = &tileset->tile_data[id];

        struct json_object *properties_obj = json_object_object_get(tile_obj, "properties");
        for (size_t j = 0; j < json_object_array_length(properties_obj); j++)
        {
            struct json_object *property_obj = json_object_array_get_idx(properties_obj, j);

            struct json_object *name_obj = json_object_object_get(property_obj, "name");
            const char *name = json_object_get_string(name_obj);

            if (strcmp(name, "solid") == 0)
            {
                struct json_object *value_obj = json_object_object_get(property_obj, "value");
                tile_data->solid = json_object_get_boolean(value_obj);
            }
        }
    }
}

void tileset_unload(struct tileset *tileset)
{
    printf("Unloading tileset: %s\n", tileset->filename);

    free(tileset->filename);

    free(tileset->image);

    free(tileset->tile_data);
}

struct tile_data *tileset_get_tile_data(struct tileset *tileset, size_t gid)
{
    return &tileset->tile_data[gid - tileset->first_gid];
}
