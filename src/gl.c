#include "gl.h"

#include <SDL3/SDL_video.h>

PFNGLCLEARCOLORPROC glClearColor;
PFNGLCLEARPROC glClear;

int gl_load(void) {
    void *fn;

    fn = SDL_GL_GetProcAddress("glClearColor"); if (!fn) return -1; glClearColor = fn;
    fn = SDL_GL_GetProcAddress("glClear");      if (!fn) return -1; glClear = fn;
    return 0;
}
