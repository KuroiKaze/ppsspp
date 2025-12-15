#include "bgmHandler.h"
#include <stdio.h>

extern void debug_log(const char *format, ...);

bool bgm_init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        debug_log("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }
    return true;
}

// --- BGM ---
void bgm_play(BGMHandler* bgm, const char* path, int loops) {
    if (!bgm) return;
    if (bgm->music) {
        Mix_FreeMusic(bgm->music);
        bgm->music = NULL;
    }
    bgm->music = Mix_LoadMUS(path);
    if (!bgm->music) {
        debug_log("Failed to load BGM: %s\n", Mix_GetError());
        return;
    }
    Mix_PlayMusic(bgm->music, loops);
}

void bgm_stop(BGMHandler* bgm) {
    if (!bgm) return;
    Mix_HaltMusic();
}

void bgm_cleanup(BGMHandler* bgm) {
    if (!bgm) return;
    if (bgm->music) Mix_FreeMusic(bgm->music);
    bgm->music = NULL;
    Mix_CloseAudio();
}

// --- SFX ---
bool sfx_init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer konnte nicht initialisiert werden! Fehler: %s\n", Mix_GetError());
        return false;
    }
    Mix_AllocateChannels(16);
    return true;
}

Mix_Chunk* sfx_load(const char* path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if (!chunk) {
        debug_log("SFX konnte nicht geladen werden: %s\n", Mix_GetError());
    }
    return chunk;
}

void sfx_play(Mix_Chunk* chunk, int loops) {
    if (!chunk) return;
    Mix_PlayChannel(-1, chunk, loops);
}

void sfx_cleanup(Mix_Chunk* sfx) {
    if (!sfx) return;
    Mix_FreeChunk(sfx);
}