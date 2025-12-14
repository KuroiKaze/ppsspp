#include <SDL.h>
#include <SDL_image.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "player/player.h"
#include "enemies/mummy/mummy.h"
#include "enemies/slime/slime.h"
#include "ui/ui.h"
#include "background/background.h"
#include "map/map.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

int running = 1;

// --- Debug Logger ---
void debug_log(const char *format, ...) {
    FILE *fp = fopen("host0:/crash_log.txt", "a");
    if (fp) {
        va_list args;
        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

// --- Callbacks ---
int exit_callback(int arg1, int arg2, void *common) { running = 0; return 0; }
int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
int setup_callbacks(void) {
    int thid = sceKernelCreateThread("callback_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
    return thid;
}

SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path) {
    debug_log("Load Texture: %s", path);
    SDL_Surface *pixels = IMG_Load(path);
    if (!pixels) {
        debug_log("ERROR IMG_Load: %s", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, pixels);
    if (!texture) debug_log("ERROR SDL_CreateTexture: %s", SDL_GetError());
    SDL_FreeSurface(pixels);
    return texture;
}

// --- Reset Game (Player + alle Gegner) ---
void reset_game(Player *player, Enemy **enemies, int enemy_count) {
    // Player reset
    player->entity.health = PLAYER_MAX_HEALTH;
    player->entity.rect.x = 50;
    player->entity.rect.y = 20;
    player->entity.vel_x = 0;
    player->entity.vel_y = 0;
    player->entity.current_idle_frame = 0;
    player->attack_timer_end = 0;
    player->attack_cooldown_end = 0;
    player->current_attack_frame = 0;

    for (int i = 0; i < enemy_count; i++) {
        enemies[i]->entity.health = ENEMY_MAX_HEALTH;
        enemies[i]->entity.rect.x = enemies[i]->spawn_x;
        enemies[i]->entity.rect.y = enemies[i]->spawn_y;
        enemies[i]->is_moving = 0;
        enemies[i]->entity.vel_x = 0;
        enemies[i]->entity.vel_y = 0;
    }
}

// --- Spielerangriff auf Gegner ---
void player_attack_enemy(Player *player, Enemy *enemy, int damage) {
    if (enemy->entity.health <= 0) return;
    if (SDL_HasIntersection(&player->attack_rect, &enemy->entity.rect)) {
        enemy_decrease_health(enemy, damage);
        player->attack_rect = (SDL_Rect){0,0,0,0};
    }
}

// --- Main ---
int main(int argc, char *argv[]) {
    debug_log("=== NEW SESSION START ===");

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Map level_map = {0};
    Player player = {0};

    // Gegner definieren
    Mummy mummy_enemy = {0};
    Slime slime_enemy = {0};

    BackgroundLayer layer_far_back = {0};
    BackgroundLayer layer_mid = {0};
    BackgroundLayer layer_fore = {0};

    int game_state = 0; // 0 = SPIELEN, 1 = GAME OVER
    int map_pixel_width = 0;
    int map_pixel_height = 0;

    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) goto cleanup;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) goto cleanup;

    window = SDL_CreateWindow("retro_game", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) goto cleanup;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) goto cleanup;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // --- Map & Hintergrund laden (Pfade) ---
    const char* map_path = "host0:/resources/maps/map_level1.json";
    const char* cemetery_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/tileset.png";
    const char* tower_path    = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/Add-on 1/Layers/tower_size_reduced.png";
    const char* far_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/background.png";
    const char* mid_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/mountains.png";
    const char* fore_back_path= "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/graveyard.png";
    const char* collision_path= "host0:/resources/sprites/collision_tileset.png";

    if (!map_init(&level_map, renderer, map_path, cemetery_path, tower_path, collision_path)) {
        debug_log("CRITICAL: Map init failed.");
        goto cleanup;
    }

    if (level_map.tiled_map) {
        map_pixel_width  = level_map.tiled_map->width * level_map.tiled_map->tilewidth;
        map_pixel_height = level_map.tiled_map->height * level_map.tiled_map->tileheight;
    }

    layer_far_back = background_layer_init(renderer, far_back_path, 0.005f);
    layer_mid      = background_layer_init(renderer, mid_back_path, 0.3f);
    layer_fore     = background_layer_init(renderer, fore_back_path, 0.5f);

    player = player_init(renderer);

    // Gegner initialisieren
    mummy_enemy = mummy_init(renderer, 350, 100);
    slime_enemy = slime_init(renderer, 500, 100);

    Enemy* enemies[2];
    enemies[0] = &mummy_enemy.base;
    enemies[1] = &slime_enemy.base;
    int enemy_count = sizeof(enemies) / sizeof(enemies[0]);

    SceCtrlData pad;

    // --- GAME LOOP ---
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = 0;

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_SELECT) running = 0;

        int is_moving = (player.entity.vel_x != 0);

        int camera_x = player.entity.rect.x + (player.entity.rect.w / 2) - (SCREEN_WIDTH / 2);
        int camera_y = player.entity.rect.y + (player.entity.rect.h / 2) - (SCREEN_HEIGHT / 2);
        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (map_pixel_width > 0 && camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (map_pixel_height > 0 && camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;

        if (game_state == 0) {
            player_handle_input(&player, &pad);
            player_update_physics(&player, &level_map);
            player_update_animation(&player, is_moving);
            player_update_attack(&player);

            for (int i = 0; i < enemy_count; i++) {
                player_attack_enemy(&player, enemies[i], 5);
            }

            for (int i = 0; i < enemy_count; i++) {
                enemy_update(enemies[i], &player, &level_map);
                enemy_handle_attack(enemies[i], &player);
            }

            // Spieler stirbt, wenn er unter Map fällt
            if (player.entity.rect.y > map_pixel_height + 100) {
                player_decrease_health(&player, 1000);
            }

            if (player.entity.health <= 0) game_state = 1;
        }
        else if (game_state == 1) {
            if (pad.Buttons & PSP_CTRL_START) {
                reset_game(&player, enemies, enemy_count);
                game_state = 0;
            }
        }

        // --- Render ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (game_state == 0) {
            background_layer_render(renderer, &layer_far_back, camera_x, camera_y, SCREEN_WIDTH, SCREEN_HEIGHT);
            background_layer_render(renderer, &layer_mid, camera_x, camera_y, SCREEN_WIDTH, SCREEN_HEIGHT);
            background_layer_render(renderer, &layer_fore, camera_x, camera_y, SCREEN_WIDTH, SCREEN_HEIGHT);

            map_render(renderer, &level_map, camera_x, camera_y);
            for(int i = 0; i < enemy_count; i++) {
                enemy_render(renderer, enemies[i], camera_x, camera_y);
            }
            player_render(renderer, &player, is_moving, camera_x, camera_y);
            ui_render_health_bar(renderer, player.entity.health);
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
            SDL_RenderClear(renderer);
            player_render(renderer, &player, 0, camera_x, camera_y);
        }

        SDL_RenderPresent(renderer);
    }

cleanup:
    debug_log("Cleaning up...");
    player_cleanup(&player);
    for (int i = 0; i < enemy_count; i++) enemy_cleanup(enemies[i]);
    background_layer_cleanup(&layer_far_back);
    background_layer_cleanup(&layer_mid);
    background_layer_cleanup(&layer_fore);
    map_cleanup(&level_map);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    sceKernelExitGame();
    return 0;
}