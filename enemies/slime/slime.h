#ifndef SLIME_H
#define SLIME_H

#include "../melee.h"
#include <SDL.h>

#define SLIME_MAX_HEALTH 30
#define MELEE_ATTACK_RANGE 25
#define SLIME_DAMAGE 10

typedef struct Slime {
    MeleeEnemy base;
} Slime;

Slime slime_init(SDL_Renderer *renderer, int x, int y);
#endif