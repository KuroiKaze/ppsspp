#ifndef BGM_HANDLER_H
#define BGM_HANDLER_H

#include <SDL_mixer.h>
#include <stdbool.h>

typedef struct {
    Mix_Music* music;    // Hintergrundmusik
    bool initialized;
} BGMHandler;

typedef struct {
    Mix_Chunk* chunk;    // Soundeffekte
} SFX;

// --- BGM Funktionen ---
bool bgm_init();
void bgm_play(BGMHandler* bgm, const char* path, int loops);
void bgm_stop(BGMHandler* bgm);
void bgm_cleanup(BGMHandler* bgm);

// --- SFX Funktionen ---
Mix_Chunk* sfx_load(const char* path);
bool sfx_init();
void sfx_play(Mix_Chunk* sfx, int loops);
void sfx_cleanup(Mix_Chunk* sfx);

#endif