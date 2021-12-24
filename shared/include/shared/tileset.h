#ifndef TILESET_H
#define TILESET_H

#include <stdbool.h>
#include <stdint.h>

struct tile_data
{
    bool solid;
};

struct tileset
{
    size_t index;

    char *filename;
    int64_t first_gid;

    size_t columns;
    char *image;
    struct tile_data *tile_data;
};

void tileset_load(struct tileset *tileset, char *filename);
void tileset_unload(struct tileset *tileset);

struct tile_data *tileset_get_tile_data(struct tileset *tileset, int64_t gid);

#endif
