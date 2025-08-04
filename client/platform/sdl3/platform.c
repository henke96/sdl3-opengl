#include "platform.h"
#include "../../client.h"
#include "../../x.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_Surface *window_surface = NULL;
static SDL_AudioStream *audio_stream = NULL;
static int sine_sample = 0;

#define platform_HEAP_SIZE ((ptrdiff_t)1 << 26)
static alignas(max_align_t) uint8_t platform_heap[platform_HEAP_SIZE];
static uint8_t *platform_heap_pos;

static const char *platform_cache_file_names[6] = {
    "main_file_cache.idx0",
    "main_file_cache.idx1",
    "main_file_cache.idx2",
    "main_file_cache.idx3",
    "main_file_cache.idx4",
    "main_file_cache.dat"
};
static SDL_IOStream *platform_cache_files[6];

int platform_frame_init(int32_t width, int32_t height) {
    // TODO: No audio if low mem?
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    for (int i = 0; i < 6; ++i) {
        platform_cache_files[i] = SDL_IOFromFile(platform_cache_file_names[i], "rb");
        if (platform_cache_files[i] == NULL) {
            SDL_Log("SDL_IOFromFile failed: %s", SDL_GetError());
            return -1;
        }
    }

    window = SDL_CreateWindow("Jagex", width, height, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return -1;
    }

    window_surface = SDL_GetWindowSurface(window);
    if (!window_surface) {
        SDL_Log("SDL_GetWindowSurface failed: %s", SDL_GetError());
        return -1;
    }

    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = 8000
    };
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!audio_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_ResumeAudioStreamDevice(audio_stream)) {
        SDL_Log("SDL_ResumeAudioStreamDevice failed: %s", SDL_GetError());
        return -1;
    }

    return 0;
}

void platform_print(char *text, ptrdiff_t text_len) {
    if (text_len > INT_MAX) text_len = INT_MAX;
    SDL_Log("%.*s", (int)text_len, text);
}

void platform_heap_reset(void *address) {
    platform_heap_pos = address;
}

void *platform_heap_alloc(size_t size, size_t align) {
    // Align to `align`.
    ptrdiff_t offset = platform_heap_pos - &platform_heap[0];
    platform_heap_pos += -offset & (align - 1); 

    void *ret = platform_heap_pos;
    size_t remaining = x_ARRAY_END(platform_heap) - platform_heap_pos;
    if (remaining < size) platform_abort();
    platform_heap_pos += size;
    return ret;
}

int platform_random_access_file_seek(platform_random_access_file file, int32_t pos) {
    if (SDL_SeekIO(platform_cache_files[file], pos, SDL_IO_SEEK_SET) < 0) return -1;
    return 0;
}

int32_t platform_random_access_file_length(platform_random_access_file file) {
    Sint64 size = SDL_GetIOSize(platform_cache_files[file]);
    if (size < 0 || size > INT32_MAX) platform_abort();
    return (int32_t)size;
}

int32_t platform_random_access_file_read(platform_random_access_file file, uint8_t *data, int32_t off, int32_t len) {
    size_t num_read = SDL_ReadIO(platform_cache_files[file], &data[off], len);
    if (
        num_read == 0 &&
        SDL_GetIOStatus(platform_cache_files[file]) != SDL_IO_STATUS_EOF
    ) return -1;
    return (int32_t)num_read;
}

SDL_Surface *platform_create_image(int32_t *data, int32_t width, int32_t height) {
    SDL_Surface *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_XRGB8888, data, width * sizeof(data[0]));;
    if (!surface) {
        SDL_Log("SDL_CreateSurfaceFrom failed: %s", SDL_GetError());
        platform_abort();
    }
    return surface;
}

void platform_draw_image(platform_image image, int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)width; (void)height;

    SDL_Rect dest = { .x = x, .y = y }; // width and height are ignored.
    if (!SDL_BlitSurface(image, NULL, window_surface, &dest)) {
        SDL_Log("SDL_BlitSurface failed: %s", SDL_GetError());
        platform_abort();
    }
    if (!SDL_UpdateWindowSurface(window)) { // TODO: move
        SDL_Log("SDL_UpdateWindowSurface failed: %s", SDL_GetError());
        platform_abort();
    }
}

int main(int argc, char **argv) {
    platform_heap_pos = &platform_heap[0];
    if (client_main(argc, argv) != 0) return 1;

    for (;;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: goto done;
                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.key == SDLK_ESCAPE) goto done;
                }
            }
        }

        int queued_bytes = SDL_GetAudioStreamQueued(audio_stream);
        if (queued_bytes < 0) {
            SDL_Log("SDL_GetAudioStreamQueued failed: %s", SDL_GetError());
            goto done;
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
                goto done;
            }
        }
    }
    done:
    SDL_DestroyAudioStream(audio_stream);
    SDL_DestroyWindow(window);
    return 0;
}
