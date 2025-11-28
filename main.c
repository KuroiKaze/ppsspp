#include <SDL.h>
#include <SDL_image.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "player/player.h"
#include "enemies/enemy.h"
#include "ui/ui.h"
#include "background/background.h"
#include "map/map.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

int running = 1;

// --- Debug Logger ---
void debug_log(const char *format, ...) {
    // Datei öffnen im "Append" (Anhängen) Modus
    FILE *fp = fopen("host0:/crash_log.txt", "a");

    if (fp) {
        va_list args;
        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);
        fprintf(fp, "\n");
        fclose(fp); // Wichtig: Datei sofort schließen, damit gespeichert wird!
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
    if (!texture) {
        // This is the error you were missing:
        debug_log("ERROR SDL_CreateTexture: %s", SDL_GetError());
    }

    SDL_FreeSurface(pixels);
    return texture;
}

void reset_game(Player *player, Enemy *enemy) {
    // Spieler zurücksetzen
    player->health = 100;
    player->rect.x = 50;
    player->rect.y = 20;
    player->vel_x = 0;
    player->vel_y = 0;
    player->current_idle_frame = 0;

    // Gegner zurücksetzen
    enemy->health = 50;
    enemy->rect.x = 350;
    enemy->rect.y = 50;
}

// --- Main Program ---
int main(int argc, char *argv[]) {
    // Wir brauchen kein File-Reset mehr, da wir in die Konsole schreiben
    debug_log("=== NEW SESSION START ===");

    SDL_Window * window = NULL;
    SDL_Renderer * renderer = NULL;
    Map level_map = {0};
    Player player = {0};
    Enemy mummy_enemy = {0};
    BackgroundLayer layer_far_back = {0};
    BackgroundLayer layer_mid = {0};
    BackgroundLayer layer_fore = {0};

    int game_state = 0; // 0 = SPIELEN, 1 = GAME OVER

    int map_pixel_width = 0;
    int map_pixel_height = 0;

    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    debug_log("Init SDL...");
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


    // --- PFADE ---
    const char* map_path = "host0:/resources/maps/map_level1.json"; // Dein neues JSON

    const char* cemetery_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/tileset.png";
    const char* tower_path    = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/Add-on 1/Layers/tower_size_reduced.png";


    const char* far_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/background.png";
    const char* mid_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/mountains.png";
    const char* fore_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/graveyard.png";

    const char* collision_path = "host0:/resources/sprites/collision_tileset.png";

    // --- LOAD MAP ---
    debug_log("Loading Map Level 1...");
    // Neue Init Funktion aufrufen
    if (!map_init(&level_map, renderer, map_path, cemetery_path, tower_path, collision_path)) {
        debug_log("CRITICAL: Map init failed.");
        goto cleanup;
    }

    if (level_map.tiled_map) {
        map_pixel_width = level_map.tiled_map->width * level_map.tiled_map->tilewidth;
        map_pixel_height = level_map.tiled_map->height * level_map.tiled_map->tileheight;
        debug_log("Map loaded. Size: %dx%d pixels", map_pixel_width, map_pixel_height);
    }

    layer_far_back = background_layer_init(renderer, far_back_path, 0.005f);
    layer_mid = background_layer_init(renderer, mid_back_path, 0.3f);
    layer_fore = background_layer_init(renderer, fore_back_path, 0.5f);

    debug_log("Loading Entities...");
    player = player_init(renderer);
    if (!player.idle_frames[0]) goto cleanup;

    mummy_enemy = enemy_init(renderer, 350, 100);

    SceCtrlData pad;

    // --- GAME LOOP ---
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) { if (event.type == SDL_QUIT) running = 0; }

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_SELECT) running = 0;

        int is_moving = 0;

        // 1. Calculate Camera Position FIRST (We need this for the background)
        int camera_x = player.rect.x + (player.rect.w / 2) - (SCREEN_WIDTH / 2);
        int camera_y = player.rect.y + (player.rect.h / 2) - (SCREEN_HEIGHT / 2);

        // Clamp Camera
        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (map_pixel_width > 0 && camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (map_pixel_height > 0 && camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;

        // STATE: SPIELEN
        if (game_state == 0) {
            player_handle_input_physics(&player, &pad, map_pixel_width, map_pixel_height);

            is_moving = (player.vel_x != 0);
            int player_movement_x = (player.current_x - player.prev_x);

            player_update_physics(&player, &level_map);
            player_update_animation(&player, is_moving);
            player_update_attack(&player);
            //enemy_update_animation(&mummy_enemy);

            // Gegner Logik
            if (mummy_enemy.health > 0) {
                // Neue kombinierte Funktion aufrufen!
                // Wichtig: Wir übergeben jetzt den player (&player)
                enemy_update(&mummy_enemy, &player, &level_map);
            }


            // Kampf: Spieler -> Gegner
            if (player.attack_rect.w > 0) {
                if (mummy_enemy.health > 0 && SDL_HasIntersection(&player.attack_rect, &mummy_enemy.rect)) {
                    enemy_decrease_health(&mummy_enemy, 5);
                    player.attack_rect = (SDL_Rect){0, 0, 0, 0};
                }
            }

            // Kampf: Gegner -> Spieler
            if (mummy_enemy.health > 0) {
                SDL_Rect enemy_hitbox = mummy_enemy.rect;
                enemy_hitbox.x += 4; enemy_hitbox.w -= 8;
                enemy_hitbox.y += 4; enemy_hitbox.h -= 8;

                if (SDL_HasIntersection(&player.rect, &enemy_hitbox)) {
                    int old_health = player.health;

                    player_decrease_health(&player, 10);

                    // Push Out
                    float player_center_x = player.rect.x + (player.rect.w / 2.0f);
                    float enemy_center_x = enemy_hitbox.x + (enemy_hitbox.w / 2.0f);

                    if (player_center_x < enemy_center_x) {
                        player.rect.x = enemy_hitbox.x - player.rect.w - 1;
                        if (player.health < old_health) player.vel_x = -8.0f;
                    } else {
                        player.rect.x = enemy_hitbox.x + enemy_hitbox.w + 1;
                        if (player.health < old_health) player.vel_x = 8.0f;
                    }

                    if (player.health < old_health) player.vel_y = -4.0f;
                }
            }

            if (player.rect.y > map_pixel_height + 100) {
                player_decrease_health(&player, 1000); // Kill player
            }

            if (player.health <= 0) {
                game_state = 1;
            }


        }
            // STATE: GAME OVER
        else if (game_state == 1) {
            if (pad.Buttons & PSP_CTRL_START) {
                reset_game(&player, &mummy_enemy);
                game_state = 0;
            }
        }

        // Kamera (UPDATE: No 'int' here, we update the existing variables!)
        camera_x = player.rect.x + (player.rect.w / 2) - (SCREEN_WIDTH / 2);
        camera_y = player.rect.y + (player.rect.h / 2) - (SCREEN_HEIGHT / 2);

        // Clamp Camera (Update existing variables)
        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (map_pixel_width > 0 && camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (map_pixel_height > 0 && camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;


        // Render
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
        }
        else {
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