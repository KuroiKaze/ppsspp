// enemy.c
#include "enemy.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../map/map.h" // Ensure we can see the Map struct

// Physics Constants (Same as Player for consistency)
#define ENEMY_GRAVITY 0.4f
#define ENEMY_MAX_FALL_SPEED 10.0f

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern void debug_log(const char *format, ...);

void enemy_cleanup(Enemy *enemy) {
    entity_cleanup(&enemy->entity);
}

void enemy_update(Enemy *enemy, Player *player, struct Map *map) {
    Entity *e = &enemy->entity;

    // 1. Death / Dying Check
    if (enemy->entity.is_dying) {
        entity_update_death(&enemy->entity);
        return;
    }
    if (enemy->entity.is_dead) {
        return;
    }

    // 2. AI LOGIC
    float p_center_x = player->entity.rect.x + player->entity.rect.w / 2.0f;
    float e_center_x = e->rect.x + e->rect.w / 2.0f;
    float diff_x = p_center_x - e_center_x;
    float distance = fabs(diff_x);

    e->vel_x = 0;
    enemy->is_moving = 0;

    // Face Player
    if (diff_x > 0) e->flip_direction = SDL_FLIP_NONE;
    else e->flip_direction = SDL_FLIP_HORIZONTAL;

    // Chase Player
    if (distance < DETECTION_RANGE && distance > 5.0f) {
        e->vel_x = (diff_x > 0) ? ENEMY_SPEED : -ENEMY_SPEED;
        enemy->is_moving = 1;
    }

    // 3. PHYSICS UPDATE
    // Replaced manual physics with the robust system
    entity_update_physics(e, map, ENEMY_GRAVITY, ENEMY_MAX_FALL_SPEED);

    // Death floor check
    if (e->rect.y > 600) e->health = 0;

    // 4. ANIMATION
    entity_update_animation(e, enemy->is_moving, ENEMY_ANIMATION_SPEED);
}

void enemy_render(SDL_Renderer *renderer, Enemy *enemy, int camera_x, int camera_y) {
    Entity *e = &enemy->entity;
    if (e->is_dead) return;

    SDL_Texture *current_texture = NULL;
    sfx_play(enemy->entity.grunt_sfx, -1);

    if (e->is_dying && e->death.count > 0) {
        current_texture = e->death.frames[e->current_death_frame];
        SDL_SetTextureAlphaMod(current_texture, e->alpha);
    }
    else if (enemy->is_moving && e->run.count > 0) {
        current_texture = e->run.frames[e->current_run_frame];
    }
    else if (e->idle.count > 0) {
        current_texture = e->idle.frames[e->current_idle_frame];
    }

    entity_render(renderer, e, current_texture, camera_x, camera_y);

    // Health Bar
    if (!e->is_dying && e->health > 0) {
        int bar_x = (e->rect.x - e->offset_x + e->sprite_w / 2 - ENEMY_BAR_W / 2) - camera_x;
        int bar_y = (e->rect.y - ENEMY_BAR_H - ENEMY_BAR_OFFSET_Y) - camera_y;

        SDL_Rect bar_bg = {bar_x, bar_y, ENEMY_BAR_W, ENEMY_BAR_H};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &bar_bg);

        float ratio = (float)e->health / ENEMY_MAX_HEALTH;
        SDL_Rect bar_hp = {bar_x, bar_y, (int)(ENEMY_BAR_W * ratio), ENEMY_BAR_H};
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &bar_hp);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &bar_bg);
    }
}

void enemy_decrease_health(Enemy *enemy, int amount) {
    enemy->entity.health -= amount;
    if (enemy->entity.health <= 0 && !enemy->entity.is_dying) {
        enemy->entity.is_dying = 1;
        enemy->entity.current_death_frame = 0;
        enemy->entity.alpha = 255;
        enemy->entity.vel_x = 0;
        enemy->entity.vel_y = 0;
    }
}

void enemy_handle_attack(Enemy *enemy, Player *player) {
    if (enemy->entity.health <= 0) return;

    // Hitbox Adjustment
    SDL_Rect enemy_hitbox = enemy->entity.rect;
    enemy_hitbox.x += 4; enemy_hitbox.w -= 8;
    enemy_hitbox.y += 4; enemy_hitbox.h -= 8;

    // Attack Player
    if (SDL_HasIntersection(&player->entity.rect, &enemy_hitbox)) {
        int old_health = player->entity.health;
        player_decrease_health(player, 10);

        // --- FIXED PUSHBACK ---
        float player_center_x = player->entity.rect.x + (player->entity.rect.w / 2.0f);
        float enemy_center_x = enemy_hitbox.x + (enemy_hitbox.w / 2.0f);

        // We removed the lines that manually set player->entity.rect.x
        // Now we only set Velocity, so the Physics Engine handles the wall collision.

        if (player_center_x < enemy_center_x) {
            // Push Left
            if (player->entity.health < old_health) player->entity.vel_x = -8.0f;
        } else {
            // Push Right
            if (player->entity.health < old_health) player->entity.vel_x = 8.0f;
        }

        if (player->entity.health < old_health) player->entity.vel_y = -4.0f;
    }
}

void enemy_take_damage_from_player(Enemy *enemy, SDL_Rect player_attack_rect, int damage) {
    if (enemy->entity.is_dead) return;

    if (SDL_HasIntersection(&player_attack_rect, &enemy->entity.rect)) {
        enemy->entity.health -= damage;
        if (enemy->entity.health <= 0 && !enemy->entity.is_dying) {
            enemy->entity.is_dying = 1;
            enemy->entity.current_death_frame = 0;
            enemy->entity.alpha = 255;
            enemy->entity.vel_x = 0;
            enemy->entity.vel_y = 0;
        }
    }
}