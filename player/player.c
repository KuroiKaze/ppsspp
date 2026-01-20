#include "player.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include "../map/map.h"

#define PLAYER_ATTACK_BASE_PATH "resources/sprites/player/attack/frame"
#define PLAYER_IDLE_BASE_PATH   "resources/sprites/player/idle/hero-idle-"
#define PLAYER_RUN_BASE_PATH    "resources/sprites/player/run/hero-run-"
#define PLAYER_HURT_BASE_PATH   "resources/sprites/player/hero-hurt.png"
#define PLAYER_JUMP_BASE_PATH   "resources/sprites/player/jump/hero-jump-"

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern void debug_log(const char *format, ...);

#define HIT_BOX_SCALE_W 0.3f
#define HIT_BOX_SCALE_H 0.8f
#define ATTACK_HITBOX_WIDTH  40
#define ATTACK_HITBOX_HEIGHT 20
#define ATTACK_HITBOX_OFFSET_Y 5
#define ATTACK_OVERLAP 15

#define GRAVITY 0.4f
#define JUMP_FORCE 9.0f
#define MAX_FALL_SPEED 10.0f
#define ANIMATION_SPEED 85  // ms pro Frame

// -------------------------------------------------------------
// Cleanup
// -------------------------------------------------------------
void player_cleanup(Player *player) {
    entity_cleanup(&player->entity);
}

// -------------------------------------------------------------
// Init
// -------------------------------------------------------------
Player player_init(SDL_Renderer *renderer) {
    Player player = {0};
    Entity *e = &player.entity;
    
    if (!entity_load_frames(renderer, &e->idle, PLAYER_IDLE_BASE_PATH)) goto fail;
    if (!entity_load_frames(renderer, &e->run, PLAYER_RUN_BASE_PATH)) goto fail;
    if (!entity_load_frames(renderer, &e->attack, PLAYER_ATTACK_BASE_PATH)) goto fail;
    if (!entity_load_frame(renderer, &e->hurt, PLAYER_HURT_BASE_PATH)) goto fail;
    if (!entity_load_frames(renderer, &e->jump, PLAYER_JUMP_BASE_PATH)) goto fail;

    SDL_QueryTexture(e->idle.frames[0], NULL, NULL, &e->sprite_w, &e->sprite_h);
    //hitbox
    e->rect.w = (int)(e->sprite_w * HIT_BOX_SCALE_W);
    e->rect.h = (int)(e->sprite_h * HIT_BOX_SCALE_H);

    e->offset_x = e->sprite_w / 2 - e->rect.w / 2;
    e->offset_y = e->sprite_h - e->rect.h;

    // position
    e->rect.x = 50;
    e->rect.y = 50;

    player.attack_timer_end = 0;
    player.attack_cooldown_end = 0;
    player.hurt_timer_end = 0;
    player.prev_buttons = 0;
    e->last_time = SDL_GetTicks() - ANIMATION_SPEED; // prevent double frame at start

    e->health = PLAYER_MAX_HEALTH;
    e->movement_speed = PLAYER_MOVEMENT_SPEED;
    e->vel_x = 0;
    e->vel_y = 0;
    e->on_ground = 0;
    e->flip_direction = SDL_FLIP_NONE;
    e->last_time = SDL_GetTicks();
    //e->attack_sfx = Mix_LoadWAV("host0:/resources/sfx/blade_draw-99885.wav");
    //if (!e->attack_sfx) goto fail;

    return player;

fail:
    entity_cleanup(e);
    Player empty = {0};
    return empty;
}

// -------------------------------------------------------------
// Input Handling
// -------------------------------------------------------------
void player_handle_input(Player *player, const SceCtrlData *pad) {
    Entity *e = &player->entity;

    if ((int)SDL_GetTicks() < (int)player->attack_timer_end) {
        e->vel_x = 0;
        return;
    }

    if ((pad->Buttons & PSP_CTRL_CIRCLE) && SDL_GetTicks() >= player->attack_cooldown_end) {
        player->attack_timer_end = SDL_GetTicks() + ATTACK_DURATION;
        player->attack_cooldown_end = SDL_GetTicks() + ATTACK_COOLDOWN;
        player->current_attack_frame = 0;
        e->vel_x = 0;
        return;
    }

    e->vel_x = 0;
    if (pad->Buttons & PSP_CTRL_LEFT)  { e->vel_x = -PLAYER_MOVEMENT_SPEED; e->flip_direction = SDL_FLIP_HORIZONTAL; }
    if (pad->Buttons & PSP_CTRL_RIGHT) { e->vel_x =  PLAYER_MOVEMENT_SPEED; e->flip_direction = SDL_FLIP_NONE; }

    int cross_just_pressed = (pad->Buttons & PSP_CTRL_CROSS) && !(player->prev_buttons & PSP_CTRL_CROSS);

    if (cross_just_pressed && e->on_ground) {
        e->vel_y = -JUMP_FORCE;
        e->on_ground = 0;
    }
    player->prev_buttons = pad->Buttons;

    // Debug Schaden
    if (pad->Buttons & PSP_CTRL_LTRIGGER) {
        player_decrease_health(player, 1);
    }
}

void player_update_attack(Player *player) {
    Uint32 t = SDL_GetTicks();
    if (t >= player->attack_timer_end) {
        player->attack_rect = (SDL_Rect){0,0,0,0};
        player->attack_sfx_played = false; // Reset für nächsten Angriff
        return;
    }

    Uint32 time_since_attack = t - (player->attack_timer_end - ATTACK_DURATION);
    int frame_duration = ATTACK_DURATION / player->entity.attack.count;
    player->current_attack_frame = time_since_attack / frame_duration;
    if (player->current_attack_frame >= player->entity.attack.count)
        player->current_attack_frame = player->entity.attack.count - 1;

    // Attack hitbox berechnen
    if (player->current_attack_frame == 1) {
        if (!player->attack_sfx_played) {
            sfx_play(player->entity.attack_sfx, 0); // nur einmal abspielen
            player->attack_sfx_played = true;
        }

        if (player->entity.flip_direction == SDL_FLIP_NONE) {
            player->attack_rect.x = player->entity.rect.x + player->entity.rect.w - ATTACK_OVERLAP;
        } else {
            player->attack_rect.x = (player->entity.rect.x + ATTACK_OVERLAP) - ATTACK_HITBOX_WIDTH;
        }
        player->attack_rect.y = player->entity.rect.y + player->entity.rect.h / 2 - ATTACK_HITBOX_HEIGHT/2 + ATTACK_HITBOX_OFFSET_Y;
        player->attack_rect.w = ATTACK_HITBOX_WIDTH;
        player->attack_rect.h = ATTACK_HITBOX_HEIGHT;

    } else {
        player->attack_rect = (SDL_Rect){0,0,0,0};
    }
}

void player_decrease_health(Player *player, int amount) {
    if (SDL_GetTicks() < player->hurt_timer_end) return;
    player->entity.health -= amount;
    player->hurt_timer_end = SDL_GetTicks() + HURT_DURATION;
    if (player->entity.health < 0) player->entity.health = 0;
}

void player_update_animation(Player *player, int is_moving) {
    entity_update_animation(&player->entity, is_moving, ANIMATION_SPEED);
}

void player_update_physics(Player *p, struct Map *map){
    entity_update_physics(&p->entity, map, GRAVITY, MAX_FALL_SPEED);
}

void player_render(SDL_Renderer *renderer, Player *player, int is_moving, int camera_x, int camera_y) {
    Entity *e = &player->entity;
    SDL_Texture *current_texture = NULL;
    Uint32 t = SDL_GetTicks();

    if (!e->on_ground && e->jump.count > 0) {
        current_texture = e->jump.frames[e->current_jump_frame];
    }
    else if (SDL_GetTicks() < player->attack_timer_end && e->attack.count > 0) {
        current_texture = e->attack.frames[player->current_attack_frame];
    }
    else if (SDL_GetTicks() < player->hurt_timer_end && e->hurt.count > 0) {
        current_texture = e->hurt.frames[0];
    }
    else {
        if (is_moving && e->run.count > 0) {
            current_texture = e->run.frames[e->current_run_frame];
        } else if (!is_moving && e->idle.count > 0) {
            current_texture = e->idle.frames[e->current_idle_frame];
        }
    }

    if (!current_texture) return;

    SDL_Rect render_rect = {
    e->rect.x - e->offset_x - camera_x,
    e->rect.y - e->offset_y - camera_y,
    e->sprite_w,
    e->sprite_h
};

    // --- Hurt Blinking ---
    SDL_SetTextureColorMod(current_texture, 255, 255, 255);
    if (t < player->hurt_timer_end && t % 100 < 50) {
        SDL_SetTextureColorMod(current_texture, 255, 100, 100);
    }

    // --- Render Copy ---
    SDL_RenderCopyEx(renderer, current_texture, NULL, &render_rect, 0.0, NULL, e->flip_direction);

#ifdef DEBUG_DRAW_HITBOX
    Uint8 old_r, old_g, old_b, old_a;
    SDL_GetRenderDrawColor(renderer, &old_r, &old_g, &old_b, &old_a);

    SDL_Rect debug_rect = e->rect;
    debug_rect.x -= camera_x;
    debug_rect.y -= camera_y;

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &debug_rect);

    SDL_SetRenderDrawColor(renderer, old_r, old_g, old_b, old_a);
#endif
}