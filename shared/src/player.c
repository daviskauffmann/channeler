#include <shared/player.h>

#include <malloc.h>
#include <math.h>
#include <shared/dialogs.h>
#include <shared/map.h>
#include <shared/tileset.h>
#include <stdbool.h>
#include <string.h>

void player_init(struct player *player, size_t map_index)
{
    player->map_index = map_index;
    player->pos_x = 100;
    player->pos_y = 100;
    player->vel_x = 0;
    player->vel_y = 0;
    player->acc_x = 0;
    player->acc_y = 0;

    player->dialog_index = 0;
    player->dialog_message_index = 0;

    player->num_quest_statuses = 0;
    player->quest_statuses = NULL;
}

void player_uninit(struct player *player)
{
    if (player->quest_statuses)
    {
        free(player->quest_statuses);
    }
}

void player_accelerate(struct player *player, struct map *map, float delta_time)
{
    float speed = 2000;
    float drag = 20;

    float acc_len = sqrtf(powf(player->acc_x, 2) + powf(player->acc_y, 2));
    if (acc_len > 1)
    {
        player->acc_x /= acc_len;
        player->acc_y /= acc_len;
    }

    player->acc_x *= speed;
    player->acc_y *= speed;

    player->acc_x -= player->vel_x * drag;
    player->acc_y -= player->vel_y * drag;

    float new_pos_x = 0.5f * player->acc_x * powf(delta_time, 2) + player->vel_x * delta_time + player->pos_x;
    float new_pos_y = 0.5f * player->acc_y * powf(delta_time, 2) + player->vel_y * delta_time + player->pos_y;

    int tile_nx_x = (int)roundf(new_pos_x / map->tile_width);
    int tile_nx_y = (int)roundf(player->pos_y / map->tile_height);
    struct tile *tile_nx = map_get_tile(map, tile_nx_x, tile_nx_y);
    if (tile_nx)
    {
        struct tileset *tileset = map_get_tileset(map, tile_nx->gid);
        struct tile_data *tile_data = tileset_get_tile_data(tileset, tile_nx->gid);
        if (tile_data->solid)
        {
            player->vel_x = 0;
        }
        else
        {
            player->pos_x = new_pos_x;
            player->vel_x = player->acc_x * delta_time + player->vel_x;
        }
    }
    else
    {
        player->vel_x = 0;
    }

    int tile_ny_x = (int)roundf(player->pos_x / map->tile_width);
    int tile_ny_y = (int)roundf(new_pos_y / map->tile_height);
    struct tile *tile_ny = map_get_tile(map, tile_ny_x, tile_ny_y);
    if (tile_ny)
    {
        struct tileset *tileset = map_get_tileset(map, tile_ny->gid);
        struct tile_data *tile_data = tileset_get_tile_data(tileset, tile_ny->gid);
        if (tile_data->solid)
        {
            player->vel_y = 0;
        }
        else
        {
            player->pos_y = new_pos_y;
            player->vel_y = player->acc_y * delta_time + player->vel_y;
        }
    }
    else
    {
        player->vel_y = 0;
    }

    player->acc_x = 0;
    player->acc_y = 0;
}

void player_attack(struct player *player, struct map *map)
{
    for (size_t i = 0; i < MAX_MOBS; i++)
    {
        struct mob *mob = &map->mobs[i];
        if (mob->alive)
        {
            float distance = sqrtf(powf(player->pos_x - mob->x, 2) + powf(player->pos_y - mob->y, 2));
            if (distance < 20)
            {
                mob->alive = false;
                mob->respawn_timer = 5;
            }
        }
    }
}

void player_advance_dialog(struct player *player, struct dialogs *dialogs)
{
    struct dialog *dialog = &dialogs->dialogs[player->dialog_index];
    struct dialog_message *message = &dialog->messages[player->dialog_message_index];
    if (!message->num_choices)
    {
        player->dialog_message_index++;
    }
}

void player_choose_dialog(struct player *player, struct dialogs *dialogs, size_t choice_index)
{
    struct dialog *dialog = &dialogs->dialogs[player->dialog_index];
    struct dialog_message *message = &dialog->messages[player->dialog_message_index];
    if (message->num_choices > choice_index)
    {
        struct dialog_choice *choice = &message->choices[choice_index];
        if (choice->outcomes.set_dialog_index != (size_t)-1)
        {
            player->dialog_index = choice->outcomes.set_dialog_index;
            player->dialog_message_index = 0;
        }
        if (choice->outcomes.set_quest_status.quest_index != (size_t)-1 && choice->outcomes.set_quest_status.stage_index != (size_t)-1)
        {
            player_set_quest_status(player, choice->outcomes.set_quest_status.quest_index, choice->outcomes.set_quest_status.stage_index);
        }
    }
}

void player_set_quest_status(struct player *player, size_t quest_index, size_t stage_index)
{
    for (size_t i = 0; i < player->num_quest_statuses; i++)
    {
        struct quest_status *status = &player->quest_statuses[i];
        if (status->quest_index == quest_index)
        {
            status->stage_index = stage_index;
            return;
        }
    }

    size_t quest_status_index = player->num_quest_statuses++;
    player->quest_statuses = realloc(player->quest_statuses, player->num_quest_statuses * sizeof(*player->quest_statuses));
    player->quest_statuses[quest_status_index].quest_index = quest_index;
    player->quest_statuses[quest_status_index].stage_index = stage_index;
}
