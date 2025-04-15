#include <inttypes.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <GL/glcorearb.h>

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    SDL_Window *window = NULL;
    SDL_GLContext context = NULL;
    SDL_AudioStream *audio_stream = NULL;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    window = SDL_CreateWindow(
        "SDL3 OpenGL",
        800, 600,
        SDL_WINDOW_OPENGL
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        goto out;
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        goto out;
    }

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = 8000
    };
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!audio_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        goto out;
    }

    PFNGLCLEARCOLORPROC glClearColor = (PFNGLCLEARCOLORPROC)SDL_GL_GetProcAddress("glClearColor");
    PFNGLCLEARPROC glClear = (PFNGLCLEARPROC)SDL_GL_GetProcAddress("glClear");
    if (!glClearColor || !glClear) {
        SDL_Log("SDL_GL_GetProcAddress failed: %s", SDL_GetError());
        goto out;
    }

    if (!SDL_ResumeAudioStreamDevice(audio_stream)) {
        SDL_Log("SDL_ResumeAudioStreamDevice failed: %s", SDL_GetError());
        goto out;
    }

    int sine_sample = 0;
    for (;;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    ret = EXIT_SUCCESS;
                    goto out;
                }
                case SDL_EVENT_KEY_UP: {
                    if (event.key.key == SDLK_ESCAPE) {
                        ret = EXIT_SUCCESS;
                        goto out;
                    }
                    break;
                }
            }
        }

        int queued_bytes = SDL_GetAudioStreamQueued(audio_stream);
        if (queued_bytes < 0) {
            SDL_Log("SDL_GetAudioStreamQueued failed: %s", SDL_GetError());
            goto out;
        }
        if ((size_t)queued_bytes < 4000 * sizeof(float)) {
            float samples[512];

            for (size_t i = 0; i < SDL_arraysize(samples); ++i) {
                float freq = 110.0f;
                float phase = (float)sine_sample * freq / 8000.0f;
                samples[i] = SDL_sinf(phase * 2 * SDL_PI_F) / 20.0f;
                sine_sample++;
                if (sine_sample >= 8000) sine_sample = 0;
            }

            if (!SDL_PutAudioStreamData(audio_stream, samples, sizeof(samples))) {
                SDL_Log("SDL_PutAudioStreamData failed: %s", SDL_GetError());
                goto out;
            }
        }

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (!SDL_GL_SwapWindow(window)) {
            SDL_Log("SDL_GL_SwapWindow failed: %s", SDL_GetError());
            goto out;
        }
    }

    out:
    SDL_DestroyAudioStream(audio_stream);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return ret;
}
