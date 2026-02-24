#include "shurikenDude.h"
#include <SDL_image.h>

#define ATTACK_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/Shuriken Dude Enemy/sprites/shuriken-dude/shuriken-dude"
#define IDLE_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/Shuriken Dude Enemy/sprites/shuriken-dude/shuriken-dude1.png"

#define PROJECTILE_BASE_PATH "resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/Shuriken Dude Enemy/sprites/shuriken/shuriken"
#define HIT_BOX_SCALE_W 0.5f
#define HIT_BOX_SCALE_H 0.8f

ShurikenDude shurikenDude_init(SDL_Renderer *renderer, int x, int y) {
    ShurikenDude sd = {0};
    Enemy *base = &sd.base.base;
    Entity *entity = &base->entity;

    // 1. Stats
    base->attack_type = RANGED;
    entity->health = SHURIKENDUDE_MAX_HEALTH;
    base->max_health = SHURIKENDUDE_MAX_HEALTH;

    sd.base.proj_vel_x = 4.0f;   // speed of projectile
    sd.base.proj_vel_y = 0.0f;
    sd.base.proj_damage = 10;
    sd.base.cooldown_ms = 2000;
    sd.base.shoot_cooldown_end = 0;

    entity_load_frames(renderer, &entity->attack, ATTACK_BASE_PATH);
    entity_load_frame(renderer, &entity->idle, IDLE_BASE_PATH); // load single frame from attack into idle
    entity_load_frame(renderer, &entity->run, IDLE_BASE_PATH);  // no idle or run animation

    if (entity->idle.count > 0) {
        SDL_QueryTexture(entity->idle.frames[0], NULL, NULL, &entity->sprite_w, &entity->sprite_h);
    }

    // load projectile frames
    entity_load_frames(renderer, &sd.base.projectiles[0].sprite, PROJECTILE_BASE_PATH);

    if (sd.base.projectiles[0].sprite.count == 0) {
        printf("Error: no projectile frames found at %s\n", PROJECTILE_BASE_PATH);
    } else {
        sd.base.projectiles[0].rect.w = sd.base.projectiles[0].sprite.sprite_w * 1.5f;
        sd.base.projectiles[0].rect.h = sd.base.projectiles[0].sprite.sprite_h * 1.5f;
        
        // copy projectile sprite info to all other slots
        // instead of loading the same frames multiple times
        for (int i = 1; i < MAX_PROJECTILES; i++) {
            sd.base.projectiles[i].sprite = sd.base.projectiles[0].sprite;
            sd.base.projectiles[i].rect.w = sd.base.projectiles[0].rect.w;
            sd.base.projectiles[i].rect.h = sd.base.projectiles[0].rect.h;
            sd.base.projectiles[i].active = false;
        }
    }

    entity->rect.w = (int)(entity->sprite_w * HIT_BOX_SCALE_W);
    entity->rect.h = (int)(entity->sprite_h * HIT_BOX_SCALE_H);
    entity->rect.x = x;
    entity->rect.y = y;
    entity->offset_x = (entity->sprite_w - entity->rect.w) / 2;
    entity->offset_y = entity->sprite_h - entity->rect.h;

    entity->last_time = SDL_GetTicks();
    entity->flip_direction = SDL_FLIP_NONE;

    return sd;
}