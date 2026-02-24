#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <SDL.h>
#include "../player/player.h"
#include "../entity/spriteFramesArray.h"

typedef struct {
    SDL_Rect rect;
    float x, y; // exact position as floats for smooth movement
    float vel_x, vel_y;
    int damage;
    bool active;
    SpriteFrameArray sprite;
    int current_frame;
} Projectile;

// projectile.h Korrektur
void projectiles_update(Projectile projectiles[], Player *player);
void projectiles_render(SDL_Renderer *renderer, Projectile projectiles[], int camera_x, int camera_y);

#endif