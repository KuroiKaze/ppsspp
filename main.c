#include <SDL.h>
#include <SDL_image.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudiolib.h>
#include <pspaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "player/player.h"
#include "level/level.h"
#include "level/levelHandler.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

int running = 1;

SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *pixels = IMG_Load(path);
    if (!pixels)
    {
        fprintf(stderr, "ERROR IMG_Load: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, pixels);
    if (!texture)
        fprintf(stderr, "ERROR SDL_CreateTexture: %s\n", SDL_GetError());
    SDL_FreeSurface(pixels);
    return texture;
}

// --- Debug Logger ---
void debug_log(const char *format, ...)
{
    // Wir erzwingen den Pfad auf den Memory Stick (Root)
    FILE *fp = fopen("ms0:/crash_log.txt", "a"); 
    if (fp)
    {
        va_list args;
        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

// --- PSP Callbacks ---
int exit_callback(int arg1, int arg2, void *common)
{
    running = 0;
    return 0;
}
int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
int setup_callbacks(void)
{
    int thid = sceKernelCreateThread("callback_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

int main(int argc, char *argv[])
{
    FILE *f = fopen("ms0:/crash_log.txt", "w"); // empty log
    if (f) {
        fprintf(f, "=== NEW DEBUG SESSION START ===\n");
        fclose(f);
    }
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Player player = {0};

    int game_state = 0; // 0 = PLAYING, 1 = GAME OVER

    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    chdir("disc0:/PSP_GAME/USRDIR/");
    sceKernelDelayThread(1000000); // 0,5 Sekunden warten (500ms)

    debug_log("Init SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
        goto cleanup;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
        goto cleanup;

    window = SDL_CreateWindow("retro_game", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window)
        goto cleanup;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer)
            goto cleanup;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    if (chdir("disc0:/PSP_GAME/USRDIR/") != 0) {
        debug_log("Konnte Verzeichnis nicht wechseln!");
    }

    player = player_init(renderer);

    // Initialize the Handler
    LevelHandler level_handler = level_handler_init(renderer, &player);

    SceCtrlData pad;

    // --- GAME LOOP ---
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = 0;

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_SELECT) {
            level_handler_change_level(&level_handler, 0);
        }

        int is_moving = (player.entity.vel_x != 0);

        Level *level = &level_handler.current_level;

        int map_pixel_width = level->map.tiled_map->width * level->map.tiled_map->tilewidth;
        int map_pixel_height = level->map.tiled_map->height * level->map.tiled_map->tileheight;

        int camera_x = player.entity.rect.x + (player.entity.rect.w / 2) - (SCREEN_WIDTH / 2);
        int camera_y = player.entity.rect.y + (player.entity.rect.h / 2) - (SCREEN_HEIGHT / 2);

        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;
        
        if (game_state == 0) {
            if (level && level->map.tiled_map) {
                level_handler_update(&level_handler, &pad);

            // Fall Death check - Nur wenn die Map eine Höhe hat!
            if (map_pixel_height > 0 && player.entity.rect.y > map_pixel_height + 100) {
                player_decrease_health(&player, 1000);
            }
        }
            if (player.entity.health <= 0) game_state = 1;
        } else if (game_state == 1) {
            if (pad.Buttons & PSP_CTRL_START)
            {
                // Reset using the handler's current level
                level_reset(&level_handler.current_level);
                player.entity.health = PLAYER_MAX_HEALTH;
                game_state = 0;
            }
        }

        // --- Render ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (game_state == 0)
        {
            // Render via Handler
            level_handler_render(&level_handler, camera_x, camera_y);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
            SDL_RenderClear(renderer);
            // Render player manually on Game Over screen if needed
            player_render(renderer, &player, is_moving, camera_x, camera_y);
        }
        SDL_RenderPresent(renderer);
    }

    cleanup:
    debug_log("Cleaning up...");
    audio_cleanup();
    level_handler_cleanup(&level_handler);
    player_cleanup(&player);

    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    sceKernelExitGame();
    return 0;
}