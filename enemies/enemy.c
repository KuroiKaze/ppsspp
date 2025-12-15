// enemy.c
#include "enemy.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);
extern int map_get_floor_height(struct Map *map, int x, int y);
extern int map_is_solid(struct Map *map, int x, int y);
extern void debug_log(const char *format, ...);

void enemy_cleanup(Enemy *enemy) {
    entity_cleanup(&enemy->entity);
}

void enemy_update(Enemy *enemy, Player *player, struct Map *map) {
    Entity *e = &enemy->entity;
    if (enemy->entity.is_dying) {
        entity_update_death(&enemy->entity);
        return;
    }

    if (enemy->entity.is_dead) {
        return;
    }

    // --- 1. AI LOGIK ---
    float p_center_x = player->entity.rect.x + player->entity.rect.w / 2.0f;
    float e_center_x = e->rect.x + e->rect.w / 2.0f;
    float diff_x = p_center_x - e_center_x;
    float distance = fabs(diff_x);

    e->vel_x = 0;
    enemy->is_moving = 0;

    if (diff_x > 0) e->flip_direction = SDL_FLIP_NONE;
    else e->flip_direction = SDL_FLIP_HORIZONTAL;

    if (distance < DETECTION_RANGE && distance > 5.0f) {
        e->vel_x = (diff_x > 0) ? ENEMY_SPEED : -ENEMY_SPEED;
        enemy->is_moving = 1;
    }

    // --- 2. PHYSIK ---
    // X-Achse
    e->rect.x += (int)e->vel_x;
    int center_y = e->rect.y + e->rect.h / 2;
    if (e->vel_x > 0 && map_is_solid(map, e->rect.x + e->rect.w, center_y)) {
        int tile_left = ((e->rect.x + e->rect.w) / 16) * 16;
        e->rect.x = tile_left - e->rect.w;
    }
    else if (e->vel_x < 0 && map_is_solid(map, e->rect.x, center_y)) {
        int tile_right = (e->rect.x / 16) * 16 + 16;
        e->rect.x = tile_right;
    }

    // Y-Achse & Boden
    e->vel_y += GRAVITY;
    if (e->vel_y > MAX_FALL_SPEED) e->vel_y = MAX_FALL_SPEED;
    e->rect.y += (int)e->vel_y;

    int floor = map_get_floor_height(map, e->rect.x + e->rect.w / 2, e->rect.y + e->rect.h);
    e->on_ground = 0;
    if (floor != -1 && abs((e->rect.y + e->rect.h) - floor) <= 10) {
        e->rect.y = floor - e->rect.h;
        e->vel_y = 0;
        e->on_ground = 1;
    }

    if (e->rect.y > 600) e->health = 0; // Tod beim Fallen

    // --- 3. ANIMATION ---
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

    // Hitbox für Kollisionsprüfung anpassen
    SDL_Rect enemy_hitbox = enemy->entity.rect;
    enemy_hitbox.x += 4;
    enemy_hitbox.w -= 8;
    enemy_hitbox.y += 4;
    enemy_hitbox.h -= 8;

    // Angriff auf Spieler
    if (SDL_HasIntersection(&player->entity.rect, &enemy_hitbox)) {
        int old_health = player->entity.health;
        player_decrease_health(player, 10);

        // Pushback
        float player_center_x = player->entity.rect.x + (player->entity.rect.w / 2.0f);
        float enemy_center_x = enemy_hitbox.x + (enemy_hitbox.w / 2.0f);

        if (player_center_x < enemy_center_x) {
            player->entity.rect.x = enemy_hitbox.x - player->entity.rect.w - 1;
            if (player->entity.health < old_health) player->entity.vel_x = -8.0f;
        } else {
            player->entity.rect.x = enemy_hitbox.x + enemy_hitbox.w + 1;
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