#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_MOBS 20

struct tile
{
    int64_t gid;
    bool h_flip;
    bool v_flip;
    bool d_flip;
};

struct mob
{
    int64_t gid;

    float origin_x;
    float origin_y;

    float x;
    float y;

    bool alive;
    float respawn_timer;
};

struct map
{
    char *filename;

    size_t width;
    size_t height;
    struct tile *tiles; // TODO: layer support
    struct mob mobs[MAX_MOBS];

    size_t tile_width;
    size_t tile_height;

    size_t num_tilesets;
    struct tileset *tilesets;
};

void map_init(struct map *map, char *filename);
void map_uninit(struct map *map);

void map_load(struct map *map);
void map_unload(struct map *map);

void map_update(struct map *map, float delta_time);

struct tile *map_get_tile(struct map *map, size_t x, size_t y);
struct tileset *map_get_tileset(struct map *map, int64_t gid);

#endif
