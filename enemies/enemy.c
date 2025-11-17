#include "enemy.h"
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

extern SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

#define HIT_BOX_SCALE_W 0.5f
#define HIT_BOX_SCALE_H 0.8f
#define ENEMY_IDLE_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/mummy-idle/Sprites/mummy-idle-"
#define ENEMY_ANIMATION_SPEED 150
#define ENEMY_MAX_HEALTH 50 

#define ENEMY_BAR_W 50            
#define ENEMY_BAR_H 5            
#define ENEMY_BAR_OFFSET_Y 10  


Enemy enemy_init(SDL_Renderer *renderer, int initial_x, int initial_y) {
    Enemy enemy = {0};
    char path_buffer[256];

    const char* asset_root = "host0:/";

    for (int i = 0; i < ENEMY_IDLE_FRAME_COUNT; i++) {
        sprintf(path_buffer, "%s%s%d.png", asset_root, ENEMY_IDLE_BASE_PATH, i + 1);
        enemy.idle_frames[i] = load_texture(renderer, path_buffer);
        if (!enemy.idle_frames[i]) {
            fprintf(stderr, "Error: Enemy Idle Frame %d could not be loaded (%s).\n", i + 1, path_buffer);
            for (int j = 0; j <= i; j++) {
                if (enemy.idle_frames[j]) SDL_DestroyTexture(enemy.idle_frames[j]);
            }
            return enemy; 
        }
    }

    if (enemy.idle_frames[0]) {
        SDL_QueryTexture(enemy.idle_frames[0], NULL, NULL, &enemy.sprite_w, &enemy.sprite_h);
    } else {
        fprintf(stderr, "Error: First Enemy Idle Frame is NULL after loading.\n");
        return enemy;
    }

    enemy.rect.w = (int)(enemy.sprite_w * HIT_BOX_SCALE_W);
    enemy.rect.h = (int)(enemy.sprite_h * HIT_BOX_SCALE_H);

    enemy.offset_x = (enemy.sprite_w - enemy.rect.w) / 2;
    enemy.offset_y = enemy.sprite_h - enemy.rect.h; 

    enemy.rect.x = initial_x + enemy.offset_x; 
    enemy.rect.y = initial_y + enemy.offset_y; 
    
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.current_idle_frame = 0;
    enemy.last_time = SDL_GetTicks();

    return enemy;
}

void enemy_update_animation(Enemy *enemy) {
    Uint32 current_time = SDL_GetTicks();
    if (current_time > enemy->last_time + ENEMY_ANIMATION_SPEED) {
        enemy->current_idle_frame = (enemy->current_idle_frame + 1) % ENEMY_IDLE_FRAME_COUNT; 
        enemy->last_time = current_time;
    }
}

void enemy_decrease_health(Enemy *enemy, int amount) {
    enemy->health -= amount;
    if (enemy->health < 0) {
        enemy->health = 0;
    }
}


void enemy_render(SDL_Renderer *renderer, Enemy *enemy) {
    SDL_Texture *current_texture = enemy->idle_frames[enemy->current_idle_frame];
    if (enemy->health <= 0) return;

    if (!current_texture) return;

    SDL_Rect render_rect = {
        enemy->rect.x - enemy->offset_x,
        enemy->rect.y - enemy->offset_y, 
        enemy->sprite_w,                 
        enemy->sprite_h               
    };

    SDL_RenderCopy(renderer, current_texture, NULL, &render_rect);
    
    int bar_x = enemy->rect.x - enemy->offset_x + (enemy->sprite_w / 2) - (ENEMY_BAR_W / 2);
    int bar_y = enemy->rect.y - ENEMY_BAR_H - ENEMY_BAR_OFFSET_Y;

    SDL_Rect bar_background_rect = {bar_x, bar_y, ENEMY_BAR_W, ENEMY_BAR_H};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &bar_background_rect);

    float health_ratio = (float)enemy->health / (float)ENEMY_MAX_HEALTH;
    int health_width = (int)(ENEMY_BAR_W * health_ratio);

    SDL_Rect health_rect = {bar_x, bar_y, health_width, ENEMY_BAR_H};

    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255); 
    SDL_RenderFillRect(renderer, &health_rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Weißer Rahmen
    SDL_RenderDrawRect(renderer, &bar_background_rect);

    // HINWEIS ZUM NAMEN: Da in dieser Umgebung keine SDL_ttf Unterstützung verfügbar ist,
    // kann der Name des Gegners ("MUMIE") nicht als Text über der Leiste gerendert werden.

    SDL_Color original_color;
    SDL_GetRenderDrawColor(renderer, &original_color.r, &original_color.g, &original_color.b, &original_color.a);

    // Setzt die Farbe auf Rot
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); 
    
    // Zeichnet das Rechteck (die Hitbox)
    SDL_RenderDrawRect(renderer, &enemy->rect);
    
    // Setzt die Farbe zurück auf die ursprüngliche Farbe (wichtig, um den Hintergrund/andere Elemente nicht zu beeinflussen)
    SDL_SetRenderDrawColor(renderer, original_color.r, original_color.g, original_color.b, original_color.a);
}

void enemy_cleanup(Enemy *enemy) {
    for (int i = 0; i < ENEMY_IDLE_FRAME_COUNT; i++) {
        if (enemy->idle_frames[i]) {
            SDL_DestroyTexture(enemy->idle_frames[i]);
            enemy->idle_frames[i] = NULL;
        }
    }
}