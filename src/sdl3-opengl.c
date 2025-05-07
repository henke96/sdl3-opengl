#include "gl.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_GLContext context = NULL;
static SDL_AudioStream *audio_stream = NULL;
static int sine_sample = 0;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void)appstate; (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    window = SDL_CreateWindow(
        "SDL3 OpenGL",
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = 8000
    };
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!audio_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    int status = gl_load();
    if (status < 0) {
        SDL_Log("gl_load failed: %d", status);
        return SDL_APP_FAILURE;
    }

    if (!SDL_ResumeAudioStreamDevice(audio_stream)) {
        SDL_Log("SDL_ResumeAudioStreamDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    (void)appstate;

    int queued_bytes = SDL_GetAudioStreamQueued(audio_stream);
    if (queued_bytes < 0) {
        SDL_Log("SDL_GetAudioStreamQueued failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if ((size_t)queued_bytes < 4000 * sizeof(float)) {
        float samples[512];

        for (size_t i = 0; i < SDL_arraysize(samples); ++i) {
            float freq = 440.0f;
            float phase = (float)sine_sample * freq / 8000.0f;
            samples[i] = SDL_sinf(phase * 2 * SDL_PI_F) / 20.0f;
            sine_sample++;
            if (sine_sample >= 8000) sine_sample = 0;
        }

        if (!SDL_PutAudioStreamData(audio_stream, samples, sizeof(samples))) {
            SDL_Log("SDL_PutAudioStreamData failed: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!SDL_GL_SwapWindow(window)) {
        SDL_Log("SDL_GL_SwapWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    (void)appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN: {
            if (event->key.key == SDLK_ESCAPE) return SDL_APP_SUCCESS;
        }
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)appstate; (void)result;

    SDL_DestroyAudioStream(audio_stream);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
}
