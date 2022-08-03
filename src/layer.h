#ifndef LAYER_H
#define LAYER_H

#include <stdbool.h>
#include <stdlib.h>

struct tile
{
    size_t gid;
    bool h_flip;
    bool v_flip;
    bool d_flip;
};

struct layer
{
    struct tile *tiles;
};

struct tile *layer_get_tile(struct layer *layer, struct map *map, size_t x, size_t y);

#endif
