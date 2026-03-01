#ifndef SHURIKENDUDE_H
#define SHURIKENDUDE_H

#include "../ranged.h"

#define SHURIKENDUDE_MAX_HEALTH 70
#define SHURIKENDUDE_DAMAGE 20

typedef struct {
    RangedEnemy base;
} ShurikenDude;

ShurikenDude shurikenDude_init(SDL_Renderer *renderer, int x, int y);

#endif