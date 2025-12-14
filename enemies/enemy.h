#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>
#include "../entity/entity.h" // Hier wird die Entity-Struktur eingebunden
#include "../player/player.h" // Für die Player-Struktur

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

typedef struct {
    Entity entity;        // Alles aus Entity: rect, vel, frames, flip, on_ground, etc.
    int is_moving;        // Laufstatus für Animation
    int current_idle_frame;
    int current_run_frame;
    int spawn_x;         // Startposition X für Reset
    int spawn_y;         // Startposition Y für Reset
    Uint32 last_time;     // Zeitstempel für Animation
    int damage;          
} Enemy;

// Funktionsprototypen
void enemy_update_animation(Enemy *enemy);
void enemy_update(Enemy *enemy, Player *player, struct Map *map);
void enemy_decrease_health(Enemy *enemy, int amount);
void enemy_render(SDL_Renderer *renderer, Enemy *enemy, int camera_x, int camera_y);
void enemy_cleanup(Enemy *enemy);
void enemy_handle_attack(Enemy *enemy, Player *player);
void enemy_take_damage_from_player(Enemy *enemy, SDL_Rect attack_rect, int damage);

#endif // ENEMY_H