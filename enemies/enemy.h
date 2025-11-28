#ifndef ENEMY_H
#define ENEMY_H

#include <SDL.h>

#define ENEMY_IDLE_FRAME_COUNT 6

typedef struct {
    SDL_Texture *idle_frames[ENEMY_IDLE_FRAME_COUNT];
    SDL_Rect rect;    
    int sprite_w;       
    int sprite_h;  
    int offset_x;      
    int offset_y;       

    int health;          
    
    int current_idle_frame; 
    Uint32 last_time; 
} Enemy;

Enemy enemy_init(SDL_Renderer *renderer, int initial_x, int initial_y);

void enemy_update_animation(Enemy *enemy);
void enemy_decrease_health(Enemy *enemy, int amount);
void enemy_render(SDL_Renderer *renderer, Enemy *enemy, int camera_x, int camera_y);
void enemy_cleanup(Enemy *enemy);

#endif // ENEMY_H