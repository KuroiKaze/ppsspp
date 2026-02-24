#include "melee.h"

bool melee_check_Player_in_range(MeleeEnemy *enemy, Player *player) {
    float p_center_x = player->entity.rect.x + player->entity.rect.w / 2.0f;
    float e_center_x = enemy->base.entity.rect.x + enemy->base.entity.rect.w / 2.0f;
    float distance = fabs(p_center_x - e_center_x);

    return distance <= MELEE_ATTACK_RANGE;
}

void enemy_handle_melee_attack(MeleeEnemy *enemy, Player *player) {
    if (enemy->base.entity.health <= 0) return;

    // Hitbox Adjustment
    SDL_Rect enemy_hitbox = enemy->base.entity.rect;
    enemy_hitbox.x += 4; enemy_hitbox.w -= 8;
    enemy_hitbox.y += 4; enemy_hitbox.h -= 8;

    // Attack Player
    if (SDL_HasIntersection(&player->entity.rect, &enemy_hitbox)) {
        int old_health = player->entity.health;
        player_decrease_health(player, enemy->base.damage);

        // --- FIXED PUSHBACK ---
        float player_center_x = player->entity.rect.x + (player->entity.rect.w / 2.0f);
        float enemy_center_x = enemy_hitbox.x + (enemy_hitbox.w / 2.0f);

        if (player_center_x < enemy_center_x) {
            // Push Left
            if (player->entity.health < old_health) player->entity.vel_x = -8.0f;
        } else {
            // Push Right
            if (player->entity.health < old_health) player->entity.vel_x = 8.0f;
        }

        if (player->entity.health < old_health) player->entity.vel_y = -4.0f;
    }
}