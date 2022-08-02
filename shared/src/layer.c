#include <shared/layer.h>

#include <shared/map.h>

struct tile *layer_get_tile(struct layer *layer, struct map *map, size_t x, size_t y)
{
    if (x < map->width && y < map->height)
    {
        struct tile *tile = &layer->tiles[x + y * map->width];
        if (tile->gid)
        {
            return tile;
        }
    }

    return NULL;
}
