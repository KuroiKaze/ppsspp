#ifndef MUMMY_H
#define MUMMY_H

#include "../enemy.h"
#include <SDL.h>

typedef struct Mummy {
    Enemy base;  // „Vererbung“: Mummy ist ein Enemy
    int attack_cooldown; // Beispiel für mummy-spezifische Logik
} Mummy;

Mummy mummy_init(SDL_Renderer *renderer, int x, int y);
#endif