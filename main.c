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
#include "map/map.h" // Inkludiert jetzt cute_tiled.h

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
    SDL_FreeSurface(pixels);
    return texture;
}

void reset_game(Player *player, Enemy *enemy) {
    // Spieler zurücksetzen
    player->health = 100;
    player->rect.x = 50; // Startposition X
    player->rect.y = 50; // Startposition Y
    player->vel_x = 0;
    player->vel_y = 0;
    player->current_idle_frame = 0;

    // Gegner zurücksetzen
    enemy->health = 50; // Oder ENEMY_MAX_HEALTH
    enemy->rect.x = 350; // Startposition Gegner
    enemy->rect.y = 110;
}

// --- Main Program ---
int main(int argc, char *argv[]) {
    // Log resetten
    FILE *fp = fopen("host0:/crash_log.txt", "w");
    if (fp) { fprintf(fp, "=== NEW SESSION ===\n"); fclose(fp); }

    // 1. Variablen initialisieren (Nullen)
    SDL_Window * window = NULL;
    SDL_Renderer * renderer = NULL;
    Map level_map = {0};
    Player player = {0};
    Enemy mummy_enemy = {0};
    BackgroundLayer layer_far_back = {0};
    BackgroundLayer layer_mid = {0};
    BackgroundLayer layer_fore = {0};

    int game_state = 0; // 0 = SPIELEN, 1 = GAME OVER

    // Variablen für Map-Größe (werden nach dem Laden gefüllt)
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
    const char* map_path = "host0:/resources/maps/cemetery.json";
    const char* tileset_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/tileset.png";
    const char* far_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/background.png";
    const char* mid_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/mountains.png";
    const char* fore_back_path = "host0:/resources/Gothicvania Collection Files/Assets/Environments/Cemetery/base/Layers/graveyard.png";

    // --- LOAD MAP (JSON) ---
    debug_log("Loading Map JSON...");
    if (!map_init(&level_map, renderer, map_path, tileset_path)) {
        debug_log("CRITICAL: Map init failed.");
        goto cleanup;
    }

    // GRÖSSE AUSLESEN (für die Kamera später)
    if (level_map.tiled_map) {
        map_pixel_width = level_map.tiled_map->width * level_map.tiled_map->tilewidth;
        map_pixel_height = level_map.tiled_map->height * level_map.tiled_map->tileheight;
        debug_log("Map loaded. Size: %dx%d pixels", map_pixel_width, map_pixel_height);
    }

    // --- LOAD BACKGROUNDS ---
    layer_far_back = background_layer_init(renderer, far_back_path, 0.1f);
    layer_mid = background_layer_init(renderer, mid_back_path, 0.3f);
    layer_fore = background_layer_init(renderer, fore_back_path, 0.5f);

    // --- LOAD ENTITIES ---
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

        // KORREKTUR: Variable hier definieren (Standardwert 0)
        int is_moving = 0;

        // ==========================================
        // STATE: SPIEL LÄUFT
        // ==========================================
        if (game_state == 0) {
            // --- 1. LOGIK & UPDATE ---
            player_handle_input_physics(&player, &pad);

            // Hier nur noch zuweisen, NICHT neu definieren (kein "int" davor!)
            is_moving = (player.vel_x != 0);
            // Parallaxe Berechnung
            int player_movement_x = (player.current_x - player.prev_x);

            // Physik Updates
            player_update_physics(&player, &level_map);
            player_update_animation(&player, is_moving);
            player_update_attack(&player);
            enemy_update_animation(&mummy_enemy);

            // Background Updates
            background_layer_update(&layer_far_back, player_movement_x);
            background_layer_update(&layer_mid, player_movement_x);
            background_layer_update(&layer_fore, player_movement_x);

            // --- 2. KAMPF & KOLLISION ---

            // A. Spieler schlägt Gegner
            if (player.attack_rect.w > 0) {
                if (mummy_enemy.health > 0 && SDL_HasIntersection(&player.attack_rect, &mummy_enemy.rect)) {
                    enemy_decrease_health(&mummy_enemy, 5);
                    player.attack_rect = (SDL_Rect){0, 0, 0, 0}; // Nur 1 Hit pro Schlag
                }
            }

            // B. Gegner berührt Spieler (Schaden + Wegschubsen)
            if (mummy_enemy.health > 0) {
                SDL_Rect enemy_hitbox = mummy_enemy.rect;
                enemy_hitbox.x += 4; enemy_hitbox.w -= 8;
                enemy_hitbox.y += 4; enemy_hitbox.h -= 8;

                if (SDL_HasIntersection(&player.rect, &enemy_hitbox)) {
                    int old_health = player.health;

                    // Schaden nehmen
                    player_decrease_health(&player, 10);

                    // PUSH OUT LOGIK (Verhindert Überschneidung)
                    float player_center_x = player.rect.x + (player.rect.w / 2.0f);
                    float enemy_center_x = enemy_hitbox.x + (enemy_hitbox.w / 2.0f);

                    if (player_center_x < enemy_center_x) {
                        // Nach Links schieben
                        player.rect.x = enemy_hitbox.x - player.rect.w - 1;
                        if (player.health < old_health) player.vel_x = -8.0f; // Rückstoß
                    } else {
                        // Nach Rechts schieben
                        player.rect.x = enemy_hitbox.x + enemy_hitbox.w + 1;
                        if (player.health < old_health) player.vel_x = 8.0f; // Rückstoß
                    }

                    if (player.health < old_health) player.vel_y = -4.0f; // Hopser
                }
            }

            // --- 3. CHECK GAME OVER ---
            if (player.health <= 0) {
                game_state = 1; // Zustand wechseln
            }
        }
            // ==========================================
            // STATE: GAME OVER
            // ==========================================
        else if (game_state == 1) {
            // Warten auf START Taste für Reset
            if (pad.Buttons & PSP_CTRL_START) {
                reset_game(&player, &mummy_enemy);
                game_state = 0; // Zurück zum Spiel
            }
        }

        // --- KAMERA (läuft immer) ---
        int camera_x = player.rect.x + (player.rect.w / 2) - (SCREEN_WIDTH / 2);
        int camera_y = player.rect.y + (player.rect.h / 2) - (SCREEN_HEIGHT / 2);

        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (map_pixel_width > 0 && camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (map_pixel_height > 0 && camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;

        // --- RENDER START ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (game_state == 0) {
            // RENDER: NORMALES SPIEL
            background_layer_render(renderer, &layer_far_back, SCREEN_WIDTH, SCREEN_HEIGHT);
            background_layer_render(renderer, &layer_mid, SCREEN_WIDTH, SCREEN_HEIGHT);
            background_layer_render(renderer, &layer_fore, SCREEN_WIDTH, SCREEN_HEIGHT);

            map_render(renderer, &level_map, camera_x, camera_y);
            enemy_render(renderer, &mummy_enemy, camera_x, camera_y);

            // Spieler normal zeichnen
            player_render(renderer, &player, is_moving, camera_x, camera_y);
            ui_render_health_bar(renderer, player.health);
        }
        else {
            // RENDER: GAME OVER SCREEN (Dunkelrot)
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Zeichne den toten Spieler (optional)
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