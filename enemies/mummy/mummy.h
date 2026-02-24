#ifndef MUMMY_H
#define MUMMY_H

#include "../melee.h"
#include <SDL.h>

#define MUMMY_MAX_HEALTH 40
#define MELEE_ATTACK_RANGE 25
#define MUMMY_DAMAGE 15

typedef struct Mummy {
    MeleeEnemy base;
    int attack_cooldown;
} Mummy;

Mummy mummy_init(SDL_Renderer *renderer, int x, int y);
#endif