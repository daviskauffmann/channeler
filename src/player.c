#include "player.h"

#include "conversation_node.h"
#include "conversations.h"
#include "input.h"
#include "map.h"
#include "quest_status.h"
#include "tileset.h"
#include <malloc.h>
#include <math.h>
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

    player->direction = DIRECTION_DOWN;
    player->animation = ANIMATION_IDLE;
    player->animation_timer = 0;
    player->frame_index = 0;

    player->conversation_root = NULL;
    player->conversation_node = NULL;

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

void player_update(struct player *player, struct input *input, struct map *map, float delta_time)
{
    player->animation = ANIMATION_IDLE;
    if (input->dy == -1)
    {
        player->direction = DIRECTION_UP;
        player->animation = ANIMATION_WALKING;
    }
    if (input->dx == -1)
    {
        player->direction = DIRECTION_LEFT;
        player->animation = ANIMATION_WALKING;
    }
    if (input->dy == 1)
    {
        player->direction = DIRECTION_DOWN;
        player->animation = ANIMATION_WALKING;
    }
    if (input->dx == 1)
    {
        player->direction = DIRECTION_RIGHT;
        player->animation = ANIMATION_WALKING;
    }

    player->animation_timer += delta_time;
    if (player->animation_timer > 0.15f)
    {
        player->animation_timer = 0;
        player->frame_index++;
    }

    player->acc_x = (float)input->dx;
    player->acc_y = (float)input->dy;

    float acc_len = sqrtf(powf(player->acc_x, 2) + powf(player->acc_y, 2));
    if (acc_len > 1)
    {
        player->acc_x /= acc_len;
        player->acc_y /= acc_len;
    }

    float speed = 1000;
    player->acc_x *= speed;
    player->acc_y *= speed;

    float drag = 20;
    player->acc_x -= player->vel_x * drag;
    player->acc_y -= player->vel_y * drag;

    float new_pos_x = 0.5f * player->acc_x * powf(delta_time, 2) + player->vel_x * delta_time + player->pos_x;
    float new_pos_y = 0.5f * player->acc_y * powf(delta_time, 2) + player->vel_y * delta_time + player->pos_y;

    int tile_nx_x = (int)roundf(new_pos_x / map->tile_width);
    int tile_nx_y = (int)roundf(player->pos_y / map->tile_height);
    if (map_is_solid(map, tile_nx_x, tile_nx_y))
    {
        player->vel_x = 0;
    }
    else
    {
        player->pos_x = new_pos_x;
        player->vel_x = player->acc_x * delta_time + player->vel_x;
    }

    int tile_ny_x = (int)roundf(player->pos_x / map->tile_width);
    int tile_ny_y = (int)roundf(new_pos_y / map->tile_height);
    if (map_is_solid(map, tile_ny_x, tile_ny_y))
    {
        player->vel_y = 0;
    }
    else
    {
        player->pos_y = new_pos_y;
        player->vel_y = player->acc_y * delta_time + player->vel_y;
    }
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

void player_start_conversation(struct player *player, struct conversations *conversations, size_t conversation_index)
{
    player->conversation_root = player->conversation_node = &conversations->conversations[conversation_index];
    player_advance_conversation(player);
}

void player_advance_conversation(struct player *player)
{
    if (player->conversation_node->jump_id)
    {
        player->conversation_node = conversation_node_find_by_id(player->conversation_root, player->conversation_node->jump_id);
    }
    else
    {
        if (player->conversation_node->num_children)
        {
            bool has_response_nodes = false;
            for (size_t i = 0; i < player->conversation_node->num_children; i++)
            {
                struct conversation_node *child = &player->conversation_node->children[i];
                if (child->type == CONVERSATION_NODE_RESPONSE)
                {
                    has_response_nodes = true;
                    break;
                }
            }
            if (!has_response_nodes)
            {
                for (size_t i = 0; i < player->conversation_node->num_children; i++)
                {
                    struct conversation_node *child = &player->conversation_node->children[i];
                    if (conversation_node_check_conditions(child, player))
                    {
                        player->conversation_node = child;

                        if (player->conversation_node->effect.quest_status)
                        {
                            player_set_quest_status(player, *player->conversation_node->effect.quest_status);
                        }

                        break;
                    }
                }
            }
        }
        else
        {
            player->conversation_root = player->conversation_node = NULL;
        }
    }
}

void player_choose_conversation_response(struct player *player, size_t choice_index)
{
    bool has_response_nodes = false;
    for (size_t i = 0; i < player->conversation_node->num_children; i++)
    {
        struct conversation_node *child = &player->conversation_node->children[i];
        if (child->type == CONVERSATION_NODE_RESPONSE)
        {
            has_response_nodes = true;
            break;
        }
    }
    if (has_response_nodes)
    {
        size_t valid_choice_index = 0;
        for (size_t i = 0; i < player->conversation_node->num_children; i++)
        {
            struct conversation_node *child = &player->conversation_node->children[i];
            if (conversation_node_check_conditions(child, player))
            {
                valid_choice_index++;
                if (valid_choice_index == choice_index)
                {
                    player->conversation_node = child;
                    player_advance_conversation(player);
                    break;
                }
            }
        }
    }
}

void player_end_conversation(struct player *player)
{
    player->conversation_root = player->conversation_node = NULL;
}

void player_set_quest_status(struct player *player, struct quest_status quest_status)
{
    for (size_t i = 0; i < player->num_quest_statuses; i++)
    {
        struct quest_status *existing_quest_status = &player->quest_statuses[i];
        if (existing_quest_status->quest_index == quest_status.quest_index)
        {
            existing_quest_status->stage_index = quest_status.stage_index;
            return;
        }
    }

    size_t quest_status_index = player->num_quest_statuses++;
    player->quest_statuses = realloc(player->quest_statuses, player->num_quest_statuses * sizeof(*player->quest_statuses));
    player->quest_statuses[quest_status_index].quest_index = quest_status.quest_index;
    player->quest_statuses[quest_status_index].stage_index = quest_status.stage_index;
}

bool player_check_quest_status(struct player *player, struct quest_status quest_status)
{
    for (size_t i = 0; i < player->num_quest_statuses; i++)
    {
        struct quest_status *existing_quest_status = &player->quest_statuses[i];
        if (existing_quest_status->quest_index == quest_status.quest_index && existing_quest_status->stage_index == quest_status.stage_index)
        {
            return true;
        }
    }
    return false;
}
