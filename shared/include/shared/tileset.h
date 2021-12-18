#ifndef TILESET_H
#define TILESET_H

#include <stdbool.h>

struct tile_data
{
    bool solid;
};

struct tileset
{
    char *filename;

    int index;
    int columns;
    int first_gid;
    char *image;
    struct tile_data *tile_data;
};

void tileset_load(struct tileset *tileset, char *filename);
void tileset_unload(struct tileset *tileset);

struct tile_data *tileset_get_tile_data(struct tileset *tileset, int gid);

#endif
