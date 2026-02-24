// ranged.c
#include "ranged.h"

bool ranged_check_player_in_range(RangedEnemy *enemy, Player *player) {
    float dx = player->entity.rect.x - enemy->base.entity.rect.x;
    return fabs(dx) <= RANGED_ATTACK_RANGE;
}

void enemy_handle_ranged_attack(RangedEnemy *enemy, Player *player) {
    Entity *e = &enemy->base.entity;
    uint32_t now = SDL_GetTicks();

    // animation still running?
    bool is_animating = (now < enemy->base.attack_timer_end);

    if (!is_animating && now < enemy->shoot_cooldown_end) return;

    // start animation if not already running
    if (!is_animating) {
        int anim_duration = e->attack.count * ENEMY_ANIMATION_SPEED;
        enemy->base.attack_timer_end = now + anim_duration;
        e->current_attack_frame = 0;
        enemy->has_fired = false; // reset fire flag for new animation cycle
        return; 
    }

    // projectil launch timing
    int trigger_frame = e->attack.count - 4;
    if (trigger_frame < 0) trigger_frame = 0;

    if (!enemy->has_fired && e->current_attack_frame >= trigger_frame) {
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (!enemy->projectiles[i].active) {
                Projectile *p = &enemy->projectiles[i];
                p->active = true;
                
                float spawn_x = e->rect.x + (e->rect.w / 2.0f);
                float spawn_y = e->rect.y + (e->rect.h / 2.0f);

                float player_center_x = player->entity.rect.x + (player->entity.rect.w / 2.0f);
                float direction = (player_center_x > spawn_x) ? 1.0f : -1.0f;

                float launch_offset = 40.0f; 
                p->x = spawn_x + (launch_offset * direction);
                p->y = spawn_y - 10.0f;
                
                p->rect.x = (int)p->x;
                p->rect.y = (int)p->y;
                p->vel_x = enemy->proj_vel_x * direction; 
                p->vel_y = enemy->proj_vel_y; // 0 for straight, negative for upward arc
                p->damage = enemy->proj_damage;

                // Flag for only firing once per animation
                enemy->has_fired = true; 

                // cooldown for next shot starts when we fire, not at the start of the animation
                enemy->shoot_cooldown_end = now + enemy->cooldown_ms;
                break;
            }
        }
    }
}

void projectile_sprite_cleanup(RangedEnemy *enemy) {
    if (!enemy) return;
    if (enemy->projectiles[0].sprite.frames != NULL) {
        for (int i = 0; i < enemy->projectiles[0].sprite.count; i++) {
            if (enemy->projectiles[0].sprite.frames[i]) {
                SDL_DestroyTexture(enemy->projectiles[0].sprite.frames[i]);
            }
        }
        free(enemy->projectiles[0].sprite.frames);
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            enemy->projectiles[i].sprite.frames = NULL;
            enemy->projectiles[i].sprite.count = 0;
            enemy->projectiles[i].active = false;
        }
    }
}