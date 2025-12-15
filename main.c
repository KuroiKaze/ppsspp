#include <SDL.h>
#include <SDL_image.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "player/player.h"
#include "level/level.h"

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
    FILE *fp = fopen("host0:/crash_log.txt", "a");
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
    debug_log("=== NEW SESSION START ===");

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Player player = {0};

    int game_state = 0; // 0 = SPIELEN, 1 = GAME OVER
    int current_level = 0;

    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    debug_log("Init SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
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

    player = player_init(renderer);

    // --- Level initialisieren ---
    Level levels[2]; // z.B. 2 Levels
    debug_log("Loading Map Level 1...");
    level1_init(&levels[0], renderer, &player);
    // level2_init(&levels[1], renderer);

    SceCtrlData pad;
    
    // --- GAME LOOP ---
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = 0;

        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_SELECT) running = 0;

        int is_moving = (player.entity.vel_x != 0);
        Level *level = &levels[current_level];

        int map_pixel_width = level->map.tiled_map->width * level->map.tiled_map->tilewidth;
        int map_pixel_height = level->map.tiled_map->height * level->map.tiled_map->tileheight;


        int camera_x = player.entity.rect.x + (player.entity.rect.w / 2) - (SCREEN_WIDTH / 2);
        int camera_y = player.entity.rect.y + (player.entity.rect.h / 2) - (SCREEN_HEIGHT / 2);
        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (camera_x > map_pixel_width - SCREEN_WIDTH) camera_x = map_pixel_width - SCREEN_WIDTH;
        if (camera_y > map_pixel_height - SCREEN_HEIGHT) camera_y = map_pixel_height - SCREEN_HEIGHT;
    
        if (game_state == 0)
        {
            level_update(level, &pad, renderer);

            // Spieler stirbt, wenn er unter Map fällt
            if (player.entity.rect.y > level->map.tiled_map->height * 16 + 100)
            {
                player_decrease_health(&player, 1000);
            }
            if (player.entity.health <= 0) game_state = 1;
        }
        else if (game_state == 1)
        {
            if (pad.Buttons & PSP_CTRL_START)
            {
                level_reset(level);
                game_state = 0;
            }
        }

        // --- Render ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (game_state == 0)
        {
            level_render(level, renderer, camera_x, camera_y);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
            SDL_RenderClear(renderer);
            player_render(renderer, &player, is_moving, camera_x, camera_y);
        }
        SDL_RenderPresent(renderer);
    }

cleanup:
    debug_log("Cleaning up...");
    player_cleanup(&player);
    for (int i = 0; i < 2; i++)
        level_cleanup(&levels[i]);

    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    sceKernelExitGame();
    return 0;
}