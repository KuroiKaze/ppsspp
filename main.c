#include <SDL.h>
#include <SDL_image.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <stdlib.h> 
#include "player.h"

#define ANIMATION_SPEED 100  // Zeit in Millisekunden pro Frame
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

// Globale Laufvariable, wird vom Callback-Thread gesetzt
int running = 1;

// vordeklaration der Hilfsfunktion
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
    
    // necessary PSP initializations
    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    // 3. SDL Initialisierung
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "IMG Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // 4. Fenster und Renderer erstellen
    window = SDL_CreateWindow("retro_game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) goto cleanup;

    // Versuche Hardware-Renderer, Fallback auf Software, um Linker-Fehler zu umgehen
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Hardware-Renderer fehlgeschlagen. Versuch mit Software-Renderer...\n");
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) goto cleanup;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // bg
    
    Uint32 last_time = SDL_GetTicks();
    int current_idle_frame = 0;
    int current_run_frame = 0;

    Player player = player_init(renderer);
    SceCtrlData pad;
    
    // --- Hauptschleife (BEGINN) ---
    while (running) { 
        SDL_Event event;
        while (SDL_PollEvent(&event)) { if (event.type == SDL_QUIT) { running = 0; } } 

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_START || pad.Buttons & PSP_CTRL_SELECT) running = 0;

        // --- Logik-Update ---
        int is_moving = player_handle_input(&player, &pad);
        player_update_animation(&player, is_moving);

        // --- Rendering ---
        SDL_RenderClear(renderer); 
        player_render(renderer, &player, is_moving);
        SDL_RenderPresent(renderer);
    }
    
cleanup:
    player_cleanup(&player); // Spieler-Assets freigeben
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    sceKernelExitGame(); 
    return 0;
}
// ... (restlicher Code) ...

// Hilfsfunktion zum Laden einer einzelnen Textur
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