#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>

struct world
{
    size_t num_maps;
    struct map *maps;
};

void world_load(struct world *world, const char *filename, bool load_maps);
void world_unload(struct world *world);
int world_get_map_index(struct world *world, const char *filename);

#endif