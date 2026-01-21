#include "levelHandler.h"
#include <stdio.h>

extern void debug_log(const char *format, ...);

// Store texture configs for each level
const char* level1_textures[] = {
        "resources/levels/cemetery/tileset.png",
        "resources/levels/cemetery/tower.png",
};

BgConfig level1_bgs[] = {
        {"resources/levels/cemetery/background.png", 0.05f, 1.0f}, // [0] Far
        {"resources/levels/cemetery/mountains.png", 0.2f,  1.0f }, // [1] Mid
        {"resources/levels/cemetery/graveyard.png", 0.5f,  1.0f}   // [2] Fore
};

const char* level2_textures[] = {
        "resources/levels/castle/background.png",
        "resources/levels/castle/tiles.png",
        "resources/levels/castle/sprites.png"
};

BgConfig level2_bgs[] = {
        {"resources/levels/castle/back.png", 0.05f, 0.05f}, // [0] Far
        {"resources/levels/castle/mid.png", 1.0f,  1.4f }, // [1] Mid
        {NULL, 0, 0} // NULL = No foreground layer for this level
};

LevelHandler level_handler_init(SDL_Renderer* renderer, Player* player) {
    LevelHandler handler = {0};
    handler.renderer = renderer;
    handler.player = player;
    handler.current_level_index = 0;
    handler.total_levels = 2;

    handler.map_paths[0] = "resources/maps/map_level1.json";
    handler.map_paths[1] = "resources/maps/map_level2.json";

    debug_log("Handler: Loading Level 0...");
    level_load(&handler.current_level, renderer, player,
               handler.map_paths[0],
               level1_textures, 2,
               level1_bgs);

    return handler;
}

void level_handler_change_level(LevelHandler* handler, int new_index) {
    if (new_index >= handler->total_levels) return;

    level_cleanup(&handler->current_level);

    handler->current_level_index = new_index;

    debug_log("Handler: Switching to Level %d...", new_index);

    if (new_index == 0) {
        level_load(&handler->current_level, handler->renderer, handler->player,
                   handler->map_paths[0],
                   level1_textures, 2,
                   level1_bgs);
    } else if (new_index == 1) {
        level_load(&handler->current_level, handler->renderer, handler->player,
                   handler->map_paths[1],
                   level2_textures, 3,
                   level2_bgs);
    }

    // Reset Player Velocity
    handler->player->entity.vel_x = 0;
    handler->player->entity.vel_y = 0;
}

void level_handler_update(LevelHandler* handler, SceCtrlData* pad) {
    Level* lvl = &handler->current_level;
    Player* p = handler->player;

    // 1. Update the actual level logic
    level_update(lvl, pad, handler->renderer);

    // 2. Check for Door Interaction
    // We check slightly above the player's feet for a Door Tile
    int check_x = p->entity.rect.x + (p->entity.rect.w / 2);
    int check_y = p->entity.rect.y + p->entity.rect.h - 8;

    int shape = map_get_shape_at(&lvl->map, check_x, check_y);

    if (shape == SHAPE_DOOR) {
        // If Player presses UP (or whatever interact button)
        if (pad->Buttons & PSP_CTRL_UP) {
            level_handler_change_level(handler, handler->current_level_index + 1);
        }
    }
}

void level_handler_render(LevelHandler* handler, int camera_x, int camera_y) {
    level_render(&handler->current_level, handler->renderer, camera_x, camera_y);
}

void level_handler_cleanup(LevelHandler* handler) {
    level_cleanup(&handler->current_level);
}