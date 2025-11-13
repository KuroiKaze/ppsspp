#include <SDL.h>
#include <SDL_image.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <stdlib.h> 
#include "player/player.h"
#include "enemies/enemy.h"
#include "ui/ui.h"
#include "background/background.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define TARGET_FPS 60 // Hinzugefügt für Konsistenz mit PSP-Framerate

// Globale Laufvariable, wird vom Callback-Thread gesetzt
int running = 1;

// Vordeklaration der Hilfsfunktion (definiert am Ende)
SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path);

// --- PSP Callback Funktionen ---
int exit_callback(int arg1, int arg2, void *common) {
    running = 0;
    return 0;
}

int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void) {
    int thid = sceKernelCreateThread("callback_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

// --- Hauptprogramm ---
int main(int argc, char *argv[]) {
    SDL_Window * window = NULL;
    SDL_Renderer * renderer = NULL;
    Player player = {0}; 
    Enemy mummy_enemy = {0};
    
    // Initialisierungen
    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0 || !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "SDL/IMG Init Error.\n");
        goto cleanup;
    }

    window = SDL_CreateWindow("retro_game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) goto cleanup;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Hardware-Renderer fehlgeschlagen. Versuch mit Software-Renderer...\n");
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) goto cleanup;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    player = player_init(renderer); 
    mummy_enemy = enemy_init(renderer, 350, 100); 
    BackgroundLayer layer_far_back = {0};
    BackgroundLayer layer_mid = {0};
    BackgroundLayer layer_fore = {0};

    if (!player.idle_frames[0] || !player.attack_frames[0]) {
        fprintf(stderr, "Fataler Fehler: Spieler-Ressourcen konnten nicht geladen werden. Beende das Spiel.\n");
        goto cleanup; 
    }
    if (!mummy_enemy.idle_frames[0]) {
        fprintf(stderr, "Fataler Fehler: Gegner-Ressourcen konnten nicht geladen werden. Beende das Spiel.\n");
        goto cleanup; 
    }

    layer_far_back = background_layer_init(renderer, "resources/Gothicvania Collection Files/Assets/Environments/Battle Backgrounds/Pack 1/Cave-battle/PNG/back.png", 0.1f);
    layer_mid = background_layer_init(renderer, "resources/Gothicvania Collection Files/Assets/Environments/Battle Backgrounds/Pack 1/Cave-battle/PNG/middle.png", 0.3f);
    layer_fore = background_layer_init(renderer, "resources/Gothicvania Collection Files/Assets/Environments/Battle Backgrounds/Pack 1/Cave-battle/PNG/front.png", 0.5f);

    if (!layer_far_back.texture || !layer_mid.texture || !layer_fore.texture) {
        fprintf(stderr, "Fataler Fehler: Hintergrund-Ressourcen konnten nicht geladen werden.\n");
        // goto cleanup; // optional: hier das Spiel beenden
    }
    
    SceCtrlData pad;

    while (running) { 
        SDL_Event event;
        while (SDL_PollEvent(&event)) { if (event.type == SDL_QUIT) { running = 0; } } 

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_START || pad.Buttons & PSP_CTRL_SELECT) running = 0;

        int is_moving = player_handle_input(&player, &pad);
        int player_movement_x = (player.current_x - player.prev_x);
        
        player_update_animation(&player, is_moving);
        player_update_attack(&player);
        enemy_update_animation(&mummy_enemy);

        background_layer_update(&layer_far_back, player_movement_x);
        background_layer_update(&layer_mid, player_movement_x);
        background_layer_update(&layer_fore, player_movement_x);

        if (player.attack_rect.w > 0) { 
            if (mummy_enemy.health > 0 && SDL_HasIntersection(&player.attack_rect, &mummy_enemy.rect)) {
                enemy_decrease_health(&mummy_enemy, 5);
                fprintf(stderr, "Attack Hit! Enemy Health: %d\n", mummy_enemy.health);
                player.attack_rect = (SDL_Rect){0, 0, 0, 0}; 
            }
        }
        
        if (mummy_enemy.health > 0 && SDL_HasIntersection(&player.rect, &mummy_enemy.rect)) {
            player_decrease_health(&player, 8);
            
            if (player.health > 0) {
                 if (player.flip_direction == SDL_FLIP_HORIZONTAL) {
                    player.rect.x += 10; 
                } else {
                    player.rect.x -= 10; 
                }
            }
            
            if (player.health <= 0) {
                running = 0; 
            }
        }
        
        if (mummy_enemy.health <= 0) {
            // TODO: Später hier Logik für Punkte, Item-Drop etc.
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        background_layer_render(renderer, &layer_far_back, SCREEN_WIDTH, SCREEN_HEIGHT);
        background_layer_render(renderer, &layer_mid, SCREEN_WIDTH, SCREEN_HEIGHT);
        background_layer_render(renderer, &layer_fore, SCREEN_WIDTH, SCREEN_HEIGHT);

        enemy_render(renderer, &mummy_enemy); 
        player_render(renderer, &player, is_moving);

        
        ui_render_health_bar(renderer, player.health);
        
        SDL_RenderPresent(renderer);
    }
    
cleanup:
    enemy_cleanup(&mummy_enemy);
    player_cleanup(&player); 
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    sceKernelExitGame(); 
    return 0;
}

SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path) {
    SDL_Surface *pixels = IMG_Load(path);
    if (!pixels) {
        fprintf(stderr, "IMG_Load Error (%s): %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, pixels);
    SDL_FreeSurface(pixels);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    }
    return texture;
}