#include "bgmHandler.h"
#include <stdio.h>

extern void debug_log(const char *format, ...);

// Verhindert doppeltes Mix_OpenAudio
static bool g_audio_initialized = false;

bool audio_system_init() {
    if (g_audio_initialized) return true;

    // SDL Audio Subsystem
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        debug_log("SDL Audio error: %s\n", SDL_GetError());
        return false;
    }

    // Hardware öffnen - 1024 Samples Buffer ist ideal für PSP
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        debug_log("Mix_OpenAudio error: %s\n", Mix_GetError());
        return false;
    }

    Mix_AllocateChannels(16);
    g_audio_initialized = true;
    debug_log("audio ready.\n");
    return true;
}

void audio_cleanup() {
    if (g_audio_initialized) {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        g_audio_initialized = false;
        debug_log("audio end.\n");
    }
}

// --- BGM ---

bool bgm_init() {
    return audio_system_init();
}

void bgm_play(BGMHandler* bgm, const char* path, int loops) {
    if (!bgm || !g_audio_initialized) return;
    if (bgm->music) {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm->music);
        bgm->music = NULL;
    }

    bgm->music = Mix_LoadMUS(path);
    if (!bgm->music) {
        debug_log("BGM error: %s (path: %s)\n", Mix_GetError(), path);
        return;
    }

    Mix_PlayMusic(bgm->music, loops);
}

void bgm_stop(BGMHandler* bgm) {
    Mix_HaltMusic();
}

void bgm_cleanup(BGMHandler* bgm) {
    if (bgm && bgm->music) {
        Mix_FreeMusic(bgm->music);
        bgm->music = NULL;
    }
}

// --- SFX ---

bool sfx_init() {
    return audio_system_init();
}

Mix_Chunk* sfx_load(const char* path) {
    if (!g_audio_initialized) return NULL;
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if (!chunk) debug_log("SFX Fehler: %s\n", Mix_GetError());
    return chunk;
}

void sfx_play(Mix_Chunk* sfx, int loops) {
    if (sfx) Mix_PlayChannel(-1, sfx, loops);
}

void sfx_cleanup(Mix_Chunk* sfx) {
    if (sfx) Mix_FreeChunk(sfx);
}