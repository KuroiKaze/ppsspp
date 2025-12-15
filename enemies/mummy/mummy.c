#include "mummy.h"
#include <SDL_image.h>

#define IDLE_BASE_PATH "host0:/resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/mummy-idle/Sprites/mummy-idle-"
#define RUN_BASE_PATH  "host0:/resources/Gothicvania Collection Files/Assets/Characters/NPC and ENEMIES/mummy-walk/Sprites/mummy-walk-"
#define DEATH_BASE_PATH    "host0:/resources/Gothicvania Collection Files/Assets/Items, FX and Props/Blue-Enemy-Death/Sprites/Enemy-Death"
#define HIT_BOX_SCALE_W 0.5f
#define HIT_BOX_SCALE_H 0.8f

Mummy mummy_init(SDL_Renderer *renderer, int x, int y) {
    Mummy mummy = {0};
    Enemy *base = &mummy.base;
    Entity *entity = &base->entity;

    // Sprite Frames direkt über Entity laden
    entity_load_frames(renderer, &entity->idle, IDLE_BASE_PATH);
    entity_load_frames(renderer, &entity->run, RUN_BASE_PATH);
    entity_load_frames(renderer, &entity->death, DEATH_BASE_PATH);

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

    // Mummy-spezifische Werte
    mummy.attack_cooldown = 0;

    return mummy;
}