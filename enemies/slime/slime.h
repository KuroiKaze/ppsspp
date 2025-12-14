#ifndef SLIME_H
#define SLIME_H

#include "../enemy.h"
#include <SDL.h>

typedef struct Slime {
    Enemy base;
} Slime;

Slime slime_init(SDL_Renderer *renderer, int x, int y);
#endif