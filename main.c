#include <SDL.h>
#include <SDL_image.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "player/player.h"
#include "enemies/enemy.h"
#include "ui/ui.h"
#include "background/background.h"
#include "map/map.h"

#undef main

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

int running = 1;

PSP_MODULE_INFO("RETROGAME", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12288);
PSP_MAIN_THREAD_STACK_SIZE_KB(128);

// --- DEBUG HELPER (An Konsole/PSPLink senden) ---
void debug_log(const char *format, ...) {
    va_list args;
    va_start(args, format);

    // vprintf schreibt in den "Standard Output" (stdout).
    // PSPLink fängt das ab und zeigt es in deinem PC-Fenster an.
    vprintf(format, args);

    // Zeilenumbruch hinzufügen, falls im format string vergessen
    printf("\n");

    va_end(args);

    // WICHTIG: Puffer leeren, damit der Text SOFORT erscheint
    // und nicht erst, wenn der Puffer voll ist (sonst sieht man bei Crashs nix).
    fflush(stdout);
}

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
    debug_log("Load: %s", path);
    SDL_Surface *pixels = IMG_Load(path);
    if (!pixels) {
        debug_log("ERR IMG: %s", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, pixels);
    if (!texture) debug_log("ERR TEX: %s", SDL_GetError());
    SDL_FreeSurface(pixels);
    return texture;
}

void reset_game(Player *player, Enemy *enemy) {
    player->health = 100;
    player->rect.x = 50;
    player->rect.y = 20;
    player->vel_x = 0;
    player->vel_y = 0;
    player->current_idle_frame = 0;
    enemy->health = 50;
    enemy->rect.x = 350;
    enemy->rect.y = 50;
}

int main(int argc, char *argv[]) {
    pspDebugScreenInit();
    setup_callbacks();

    debug_log("=== RETROGAME START ===");

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Map level_map = {0};
    Player player = {0};
    Enemy mummy_enemy = {0};
    BackgroundLayer layer_far_back = {0};
    BackgroundLayer layer_mid = {0};
    BackgroundLayer layer_fore = {0};

    int game_state = 0;
    int map_pixel_width = 0;
    int map_pixel_height = 0;

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    debug_log("Init SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        debug_log("SDL_Init FAIL: %s", SDL_GetError());
        sceKernelDelayThread(5000000); return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        debug_log("IMG_Init FAIL: %s", IMG_GetError());
        sceKernelDelayThread(5000000); return -1;
    }

    window = SDL_CreateWindow("retro", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) { debug_log("Window FAIL"); sceKernelDelayThread(5000000); return -1; }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { debug_log("Renderer FAIL"); sceKernelDelayThread(5000000); return -1; }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    const char* map_path = "host0:/assets/map.json";

    const char* cemetery_path = "host0:/assets/tileset.png";
    const char* tower_path    = "host0:/assets/tower.png";
    const char* collision_path = "host0:/assets/collision.png";

    const char* far_back_path = "host0:/assets/bg_far.bmp";
    const char* mid_back_path = "host0:/assets/bg_mid.png";
    const char* fore_back_path = "host0:/assets/bg_fore.png";

    debug_log("Loading Map...");
    if (!map_init(&level_map, renderer, map_path, cemetery_path, tower_path, collision_path)) {
        debug_log("MAP LOAD FAILED!");
        // Optional: sceKernelDelayThread(5000000);
    }

    if (level_map.tiled_map) {
        map_pixel_width = level_map.tiled_map->width * level_map.tiled_map->tilewidth;
        map_pixel_height = level_map.tiled_map->height * level_map.tiled_map->tileheight;
    }

    debug_log("Loading Backgrounds...");
    layer_far_back = background_layer_init(renderer, far_back_path, 0.005f);
    layer_mid = background_layer_init(renderer, mid_back_path, 0.3f);
    layer_fore = background_layer_init(renderer, fore_back_path, 0.5f);

    debug_log("Loading Entities...");
    player = player_init(renderer);
    if (!player.idle_frames[0]) debug_log("PLAYER LOAD FAIL!");

    mummy_enemy = enemy_init(renderer, 350, 100);

    // --- DIAGNOSE START ---
    debug_log("--- DIAGNOSE ---");
    if (level_map.tiled_map) debug_log("Map: OK (%dx%d)", map_pixel_width, map_pixel_height);
    else debug_log("Map: NULL");

    if (player.idle_frames[0]) debug_log("Player: OK");
    else debug_log("Player: NULL");

    debug_log("Starting in 3s...");
    sceKernelDelayThread(3000000);
    // --- DIAGNOSE ENDE ---

    SceCtrlData pad;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) { if (event.type == SDL_QUIT) running = 0; }

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_SELECT) running = 0;

        int is_moving = 0;

        // Camera
        int camera_x = player.rect.x + (player.rect.w / 2) - (SCREEN_WIDTH / 2);
        int camera_y = player.rect.y + (player.rect.h / 2) - (SCREEN_HEIGHT / 2);

        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (map_pixel_width > 0 && camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (map_pixel_height > 0 && camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;

        if (game_state == 0) {
            player_handle_input_physics(&player, &pad, map_pixel_width, map_pixel_height);
            is_moving = (player.vel_x != 0);

            player_update_physics(&player, &level_map);
            player_update_animation(&player, is_moving);
            player_update_attack(&player);

            if (mummy_enemy.health > 0) {
                enemy_update(&mummy_enemy, &player, &level_map);
            }

            // Player -> Enemy
            if (player.attack_rect.w > 0) {
                if (mummy_enemy.health > 0 && SDL_HasIntersection(&player.attack_rect, &mummy_enemy.rect)) {
                    enemy_decrease_health(&mummy_enemy, 5);
                    player.attack_rect = (SDL_Rect){0, 0, 0, 0};
                }
            }

            // Enemy -> Player
            if (mummy_enemy.health > 0) {
                SDL_Rect enemy_hitbox = mummy_enemy.rect;
                enemy_hitbox.x += 4; enemy_hitbox.w -= 8;
                enemy_hitbox.y += 4; enemy_hitbox.h -= 8;

                if (SDL_HasIntersection(&player.rect, &enemy_hitbox)) {
                    int old_health = player.health;
                    player_decrease_health(&player, 10);

                    float p_cx = player.rect.x + (player.rect.w / 2.0f);
                    float e_cx = enemy_hitbox.x + (enemy_hitbox.w / 2.0f);

                    if (p_cx < e_cx) {
                        player.rect.x = enemy_hitbox.x - player.rect.w - 1;
                        if (player.health < old_health) player.vel_x = -8.0f;
                    } else {
                        player.rect.x = enemy_hitbox.x + enemy_hitbox.w + 1;
                        if (player.health < old_health) player.vel_x = 8.0f;
                    }
                    if (player.health < old_health) player.vel_y = -4.0f;
                }
            }

            // DEATH ZONE (Auskommentiert zum Testen!)
            /* if (player.rect.y > map_pixel_height + 100) player_decrease_health(&player, 1000);
            */

            if (player.health <= 0) game_state = 1;

        } else if (game_state == 1) {
            if (pad.Buttons & PSP_CTRL_START) {
                reset_game(&player, &mummy_enemy);
                game_state = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (game_state == 0) {
            background_layer_render(renderer, &layer_far_back, camera_x, camera_y, SCREEN_WIDTH, SCREEN_HEIGHT);
            background_layer_render(renderer, &layer_mid, camera_x, camera_y, SCREEN_WIDTH, SCREEN_HEIGHT);
            background_layer_render(renderer, &layer_fore, camera_x, camera_y, SCREEN_WIDTH, SCREEN_HEIGHT);

            map_render(renderer, &level_map, camera_x, camera_y);
            enemy_render(renderer, &mummy_enemy, camera_x, camera_y);
            player_render(renderer, &player, is_moving, camera_x, camera_y);
            ui_render_health_bar(renderer, player.health);
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
            SDL_RenderClear(renderer);
            player_render(renderer, &player, 0, camera_x, camera_y);
        }

        SDL_RenderPresent(renderer);
    }

    cleanup:
    debug_log("Cleaning up...");
    enemy_cleanup(&mummy_enemy);
    player_cleanup(&player);
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