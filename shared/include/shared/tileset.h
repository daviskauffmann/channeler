#ifndef TILESET_H
#define TILESET_H

#include <stdbool.h>

struct tile_data
{
    bool solid;
};

struct tileset
{
    int index;
    int columns;
    int first_gid;
    const char *image;
    struct tile_data *tile_data;
};

void tileset_load(struct tileset *tileset, const char *filename);
void tileset_unload(struct tileset *tileset);
struct tile_data *tileset_get_tile_data(struct tileset *tileset, int gid);

#endif
