#include "gl.h"

#include <SDL3/SDL_video.h>

PFNGLCLEARPROC glClear;
PFNGLCLEARCOLORPROC glClearColor;
PFNGLDISABLEPROC glDisable;
PFNGLENABLEPROC glEnable;
PFNGLGETERRORPROC glGetError;
PFNGLVIEWPORTPROC glViewport;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;

#define gl_LOAD(PROC) fn = SDL_GL_GetProcAddress(#PROC); if (!fn) return -1; PROC = fn;

int gl_load(void) {
    void *fn;

    gl_LOAD(glClear);
    gl_LOAD(glClearColor);
    gl_LOAD(glDisable);
    gl_LOAD(glEnable);
    gl_LOAD(glGetError);
    gl_LOAD(glViewport);
    gl_LOAD(glBindBuffer);
    gl_LOAD(glDeleteBuffers);
    gl_LOAD(glGenBuffers);
    gl_LOAD(glBufferData);
    gl_LOAD(glAttachShader);
    gl_LOAD(glCompileShader);
    gl_LOAD(glCreateProgram);
    gl_LOAD(glCreateShader);
    gl_LOAD(glDeleteProgram);
    gl_LOAD(glDeleteShader);
    gl_LOAD(glEnableVertexAttribArray);
    gl_LOAD(glGetUniformLocation);
    gl_LOAD(glLinkProgram);
    gl_LOAD(glShaderSource);
    gl_LOAD(glUseProgram);
    gl_LOAD(glUniformMatrix4fv);
    gl_LOAD(glVertexAttribPointer);
    gl_LOAD(glBindVertexArray);
    gl_LOAD(glDeleteVertexArrays);
    gl_LOAD(glGenVertexArrays);
    gl_LOAD(glDrawElementsInstanced);
    gl_LOAD(glVertexAttribDivisor);

    return 0;
}
