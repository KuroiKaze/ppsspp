#ifndef RANGED_H
#define RANGED_H

#include "enemy.h"
#include "projectile.h"

#define RANGED_DETECTION_RANGE 250 // doesnt really matter much
// doesn't walk towards player, just stands still and shoots when player is in range
#define RANGED_ATTACK_RANGE 200
#define MAX_PROJECTILES 2 // amount of projectiles a ranged enemy can have active at once

typedef struct {
    Enemy base;
    Projectile projectiles[MAX_PROJECTILES];
    int shoot_cooldown_end; // flag for timing next shot after animation
    float proj_vel_x;
    float proj_vel_y;
    int proj_damage;
    int cooldown_ms; // time between shots in milliseconds
    bool has_fired; // for timing projectile spawn to animation frame

} RangedEnemy;

bool ranged_check_player_in_range(RangedEnemy *enemy, Player *player);
void enemy_handle_ranged_attack(RangedEnemy *enemy, Player *player);
void projectile_sprite_cleanup(RangedEnemy *enemy);

#endif
