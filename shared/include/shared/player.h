#ifndef PLAYER_H
#define PLAYER_H

struct player
{
    size_t map_index;
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    float acc_x;
    float acc_y;
};

void player_init(struct player *player, size_t map_index);
void player_accelerate(struct player *player, struct map *map, float delta_time);
void player_attack(struct player *player, struct map *map);

#endif
