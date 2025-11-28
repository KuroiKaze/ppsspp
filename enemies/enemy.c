#include "enemy.h"
#include "../map/map.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // Für abs()

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

#define HIT_BOX_SCALE_W 0.5f
#define HIT_BOX_SCALE_H 0.8f

#define ENEMY_IDLE_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/mummy-idle/Sprites/mummy-idle-"
#define ENEMY_RUN_BASE_PATH  "resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/mummy-walk/Sprites/mummy-walk-"



#define ENEMY_ANIMATION_SPEED 150
#define ENEMY_MAX_HEALTH 50
#define ENEMY_SPEED 1.5f       // Langsamer als der Spieler (3.0f)
#define DETECTION_RANGE 250    // Ab wann jagt er dich?
#define ATTACK_RANGE 25        // Ab wann bleibt er stehen zum Schlagen?

#define GRAVITY 0.4f
#define MAX_FALL_SPEED 10.0f

#define ENEMY_BAR_W 50
#define ENEMY_BAR_H 5
#define ENEMY_BAR_OFFSET_Y 10

extern void debug_log(const char *format, ...);

Enemy enemy_init(SDL_Renderer *renderer, int initial_x, int initial_y) {
    Enemy enemy = {0};
    char path_buffer[256];
    const char* asset_root = "host0:/";

    // 1. Load IDLE
    for (int i = 0; i < ENEMY_IDLE_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%s%d.png", asset_root, ENEMY_IDLE_BASE_PATH, i + 1);
        enemy.idle_frames[i] = load_texture(renderer, path_buffer);
        if (!enemy.idle_frames[i]) debug_log("Error loading enemy idle %d", i);
    }

    // 2. Load RUN (NEU)
    for (int i = 0; i < ENEMY_RUN_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%s%d.png", asset_root, ENEMY_RUN_BASE_PATH, i + 1);
        enemy.run_frames[i] = load_texture(renderer, path_buffer);
        if (!enemy.run_frames[i]) debug_log("Error loading enemy run %d (Check Path!)", i);
    }

    if (enemy.idle_frames[0]) {
        SDL_QueryTexture(enemy.idle_frames[0], NULL, NULL, &enemy.sprite_w, &enemy.sprite_h);
    }

    enemy.rect.w = (int)(enemy.sprite_w * HIT_BOX_SCALE_W);
    enemy.rect.h = (int)(enemy.sprite_h * HIT_BOX_SCALE_H);
    enemy.offset_x = (enemy.sprite_w - enemy.rect.w) / 2;
    enemy.offset_y = enemy.sprite_h - enemy.rect.h;

    enemy.rect.x = initial_x;
    enemy.rect.y = initial_y;

    enemy.health = ENEMY_MAX_HEALTH;
    enemy.last_time = SDL_GetTicks();
    enemy.flip_direction = SDL_FLIP_NONE;

    return enemy;
}

void enemy_update(Enemy *enemy, Player *player, struct Map *map) {
    if (enemy->health <= 0) return;

    // --- 1. AI LOGIK (Verbesserte Verfolgung) ---

    // Berechne Mittelpunkte (Genauer als oben links)
    float p_center_x = player->rect.x + (player->rect.w / 2.0f);
    float e_center_x = enemy->rect.x + (enemy->rect.w / 2.0f);

    float diff_x = p_center_x - e_center_x;
    float distance = fabs(diff_x);

    // Standardwerte resetten
    enemy->vel_x = 0;
    enemy->is_moving = 0;

    // Blickrichtung immer zum Spieler (auch im Stehen)
    if (diff_x > 0) {
        enemy->flip_direction = SDL_FLIP_NONE; // Spieler ist rechts
    } else {
        enemy->flip_direction = SDL_FLIP_HORIZONTAL; // Spieler ist links
    }

    // Entscheidung: Laufen oder Angreifen?
    // DETECTION_RANGE: Er sieht dich.
    // MIN_DISTANCE: Er versucht fast in dich reinzulaufen (Aggressiv).
    // Wir nehmen 5 Pixel Toleranz, damit er nicht wild zittert, wenn er genau auf dir steht.
    float min_distance = 5.0f;

    if (distance < DETECTION_RANGE && distance > min_distance) {
        if (diff_x > 0) {
            enemy->vel_x = ENEMY_SPEED;
        } else {
            enemy->vel_x = -ENEMY_SPEED;
        }
        enemy->is_moving = 1;
    }

    // --- 2. PHYSIK (X-Achse & Wände) ---
    enemy->rect.x += (int)enemy->vel_x;

    // Wand Kollision (Kopie vom Player Code)
    int center_y = enemy->rect.y + (enemy->rect.h / 2);

    if (enemy->vel_x > 0) { // Rechts
        int right_side = enemy->rect.x + enemy->rect.w;
        if (map_is_solid(map, right_side, center_y)) {
            int tile_left = (right_side / 16) * 16;
            enemy->rect.x = tile_left - enemy->rect.w;
            // Keine Animation mehr, wenn wir gegen die Wand laufen
            // (Optional, sieht aber besser aus)
            // enemy->is_moving = 0;
        }
    }
    else if (enemy->vel_x < 0) { // Links
        int left_side = enemy->rect.x;
        if (map_is_solid(map, left_side, center_y)) {
            int tile_right = ((left_side / 16) * 16) + 16;
            enemy->rect.x = tile_right;
            // enemy->is_moving = 0;
        }
    }

    // --- 3. PHYSIK (Y-Achse & Boden) ---
    enemy->vel_y += GRAVITY;
    if (enemy->vel_y > MAX_FALL_SPEED) enemy->vel_y = MAX_FALL_SPEED;

    enemy->rect.y += (int)enemy->vel_y;

    int foot_x = enemy->rect.x + (enemy->rect.w / 2);
    int foot_y = enemy->rect.y + enemy->rect.h;
    int floor_height = map_get_floor_height(map, foot_x, foot_y);

    enemy->on_ground = 0;
    if (floor_height != -1) {
        int dist = foot_y - floor_height;
        if (dist >= -10 && dist <= 10) {
            enemy->rect.y = floor_height - enemy->rect.h;
            enemy->vel_y = 0;
            enemy->on_ground = 1;
        }
    }

    // Respawn bei Fall
    if (enemy->rect.y > 600) enemy->health = 0;

    // --- 4. ANIMATION ---
    Uint32 current_time = SDL_GetTicks();
    if (current_time > enemy->last_time + ENEMY_ANIMATION_SPEED) {
        if (enemy->is_moving) {
            enemy->current_run_frame = (enemy->current_run_frame + 1) % ENEMY_RUN_FRAME_COUNT;
        } else {
            enemy->current_idle_frame = (enemy->current_idle_frame + 1) % ENEMY_IDLE_FRAME_COUNT;
        }
        enemy->last_time = current_time;
    }
}

void enemy_decrease_health(Enemy *enemy, int amount) {
    enemy->health -= amount;
    if (enemy->health < 0) enemy->health = 0;
}

void enemy_render(SDL_Renderer *renderer, Enemy *enemy, int camera_x, int camera_y) {
    if (enemy->health <= 0) return;

    // Wähle Textur basierend auf Zustand
    SDL_Texture *current_texture;
    if (enemy->is_moving) {
        current_texture = enemy->run_frames[enemy->current_run_frame];
    } else {
        current_texture = enemy->idle_frames[enemy->current_idle_frame];
    }

    if (!current_texture) return;

    SDL_Rect render_rect = {
            (enemy->rect.x - enemy->offset_x) - camera_x,
            (enemy->rect.y - enemy->offset_y) - camera_y,
            enemy->sprite_w,
            enemy->sprite_h
    };

    // Render mit FLIP (Blickrichtung)
    SDL_RenderCopyEx(renderer, current_texture, NULL, &render_rect, 0.0, NULL, enemy->flip_direction);

    // --- Health Bar ---
    int bar_x = (enemy->rect.x - enemy->offset_x + (enemy->sprite_w / 2) - (ENEMY_BAR_W / 2)) - camera_x;
    int bar_y = (enemy->rect.y - ENEMY_BAR_H - ENEMY_BAR_OFFSET_Y) - camera_y;

    SDL_Rect bar_bg = {bar_x, bar_y, ENEMY_BAR_W, ENEMY_BAR_H};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &bar_bg);

    float ratio = (float)enemy->health / (float)ENEMY_MAX_HEALTH;
    SDL_Rect bar_hp = {bar_x, bar_y, (int)(ENEMY_BAR_W * ratio), ENEMY_BAR_H};
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bar_hp);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bar_bg);
}

void enemy_cleanup(Enemy *enemy) {
    for (int i = 0; i < ENEMY_IDLE_FRAME_COUNT; i++) {
        if (enemy->idle_frames[i]) SDL_DestroyTexture(enemy->idle_frames[i]);
    }
    // Cleanup Run Frames
    for (int i = 0; i < ENEMY_RUN_FRAME_COUNT; i++) {
        if (enemy->run_frames[i]) SDL_DestroyTexture(enemy->run_frames[i]);
    }
}