#include "projectile.h"
#include "ranged.h"

// fires a projectile -> either hits player or goes offscreen -> set used slot back to usable
void projectiles_update(Projectile projectiles[], Player *player) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;

        projectiles[i].x += projectiles[i].vel_x;
        projectiles[i].y += projectiles[i].vel_y;

        projectiles[i].rect.x = (int)projectiles[i].x;
        projectiles[i].rect.y = (int)projectiles[i].y;

        if (SDL_HasIntersection(&projectiles[i].rect, &player->entity.rect)) {
            player_decrease_health(player, projectiles[i].damage);
            projectiles[i].active = false;
            continue;
        }

        if (projectiles[i].x < 0 || projectiles[i].x > 2000) { 
            projectiles[i].active = false;
            continue;
        }
    }
}

void projectiles_render(SDL_Renderer *renderer, Projectile projectiles[], int camera_x, int camera_y) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        // check all projectiles if in active use -> render them
        if (!projectiles[i].active) continue;
        Projectile *p = &projectiles[i];

        if (p->sprite.count > 0) {
            int frame_idx = (SDL_GetTicks() / 100) % p->sprite.count;
            SDL_Texture *current_frame = p->sprite.frames[frame_idx];
            SDL_Rect dst = { 
                p->rect.x - camera_x, 
                p->rect.y - camera_y, 
                p->rect.w, 
                p->rect.h 
            };
            SDL_RenderCopy(renderer, current_frame, NULL, &dst);
        }
    }
}