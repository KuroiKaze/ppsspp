#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>
#include "../player/player.h" // Wir müssen den Player kennen!

struct Map;

#define ENEMY_IDLE_FRAME_COUNT 6
#define ENEMY_RUN_FRAME_COUNT 6 // Anzahl der Lauf-Bilder (prüf das in deinem Ordner!)

typedef struct {
    // Texturen
    SDL_Texture *idle_frames[ENEMY_IDLE_FRAME_COUNT];
    SDL_Texture *run_frames[ENEMY_RUN_FRAME_COUNT]; // NEU

    SDL_Rect rect;
    int sprite_w;
    int sprite_h;
    int offset_x;
    int offset_y;

    int health;

    // Physik & Zustand
    float vel_x;   // NEU
    float vel_y;
    int on_ground;
    int is_moving; // NEU: 0 = Idle, 1 = Run
    SDL_RendererFlip flip_direction; // NEU: Damit er sich umdreht

    // Animation
    int current_idle_frame;
    int current_run_frame; // NEU
    Uint32 last_time;
} Enemy;

Enemy enemy_init(SDL_Renderer *renderer, int initial_x, int initial_y);

// Update nimmt jetzt den Player, um ihn zu jagen!
void enemy_update(Enemy *enemy, Player *player, struct Map *map);

void enemy_decrease_health(Enemy *enemy, int amount);
void enemy_render(SDL_Renderer *renderer, Enemy *enemy, int camera_x, int camera_y);
void enemy_cleanup(Enemy *enemy);

#endif // ENEMY_H