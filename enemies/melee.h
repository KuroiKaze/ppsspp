#ifndef MELEE_H
#define MELEE_H

#include "enemy.h"

#define MELEE_DETECTION_RANGE 250 
#define MELEE_ATTACK_RANGE 25

typedef struct {
    Enemy base;
} MeleeEnemy;

bool melee_check_Player_in_range(MeleeEnemy *enemy, Player *player);
void enemy_handle_melee_attack(MeleeEnemy *enemy, Player *player);

#endif