#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>

struct world
{
    const char *filename;

    size_t num_maps;
    struct map *maps;
};

void world_load(struct world *world, const char *filename, bool load_maps);
void world_unload(struct world *world, bool unload_maps);

size_t world_get_map_index(struct world *world, const char *filename);

#endif
