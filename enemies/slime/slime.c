#include "slime.h"
#include <SDL_image.h>

#define IDLE_BASE_PATH  "resources/sprites/enemies/slime/idle/slime-idle-"
#define RUN_BASE_PATH   "resources/sprites/enemies/slime/jump/slime-jump-"
#define DEATH_BASE_PATH "resources/sprites/enemies/common/death/Enemy-Death"
#define HIT_BOX_SCALE_W 0.5f
#define HIT_BOX_SCALE_H 0.8f

Slime slime_init(SDL_Renderer *renderer, int x, int y) {
    Slime slime = {0};
    Enemy *base = &slime.base;
    Entity *entity = &base->entity;

    entity_load_frames(renderer, &entity->idle, IDLE_BASE_PATH);
    entity_load_frames(renderer, &entity->run, RUN_BASE_PATH);

    // Sprite-Größe aus erster Idle-Frame ermitteln
    if (entity->idle.count > 0 && entity->idle.frames[0]) {
        SDL_QueryTexture(entity->idle.frames[0], NULL, NULL, &entity->sprite_w, &entity->sprite_h);
    }

    // Setze Position und Größe
    entity->rect.w = (int)(entity->sprite_w * HIT_BOX_SCALE_W);
    entity->rect.h = (int)(entity->sprite_h * HIT_BOX_SCALE_H);
    entity->rect.x = x;
    entity->rect.y = y;

    // Offset für Rendering / Health Bar
    entity->offset_x = (entity->sprite_w - entity->rect.w) / 2;
    entity->offset_y = entity->sprite_h - entity->rect.h;

    // Basiswerte
    entity->health = ENEMY_MAX_HEALTH;
    entity->last_time = SDL_GetTicks();
    entity->flip_direction = SDL_FLIP_NONE;

    return slime;
}