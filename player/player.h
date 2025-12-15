#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>
#include <pspctrl.h>
#include "../entity/entity.h"

#define PLAYER_MAX_HEALTH 100
#define PLAYER_MOVEMENT_SPEED 3.0f
#define ATTACK_DURATION 300
#define ATTACK_COOLDOWN 500
#define HURT_DURATION 400
#define ATTACK_HITBOX_WIDTH 40
#define ATTACK_HITBOX_HEIGHT 20
#define ATTACK_HITBOX_OFFSET_Y 5
#define ATTACK_OVERLAP 15

struct Map;

typedef struct Player {
    Entity entity;
    Uint32 attack_timer_end;
    Uint32 attack_cooldown_end;
    Uint32 hurt_timer_end;
    int current_attack_frame;
    Uint32 prev_buttons;
    SDL_Rect attack_rect;
    bool attack_sfx_played;

} Player;

Player player_init(SDL_Renderer *renderer);
void player_handle_input(Player *player, const SceCtrlData *pad);
void player_update_attack(Player *player);
void player_update_physics(Player *p, struct Map *map);
void player_decrease_health(Player *player, int amount);
void player_update_animation(Player *player, int is_moving);
void player_render(SDL_Renderer *renderer, Player *player, int is_moving, int camera_x, int camera_y);
void player_cleanup(Player *player);

#endif