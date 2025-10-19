#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define platform_MEMSET memset
#define platform_MEMMOVE memmove
#define platform_MEMCPY memcpy
#define platform_ABORT abort
#define platform_UNREACHABLE __builtin_unreachable
#define platform_CKD_ADD32(RES, A, B) __builtin_add_overflow(A, B, RES) // TODO
#define platform_CKD_MUL32(RES, A, B) __builtin_mul_overflow(A, B, RES) // TODO

int platform_frame_init(int32_t width, int32_t height);
void platform_print(char *text, size_t text_len);
void platform_heap_reset(void *address);
void *platform_heap_alloc(int32_t len, int32_t elem_size_and_align);
int32_t platform_random(int32_t n);

int64_t platform_current_time_ns(void);
int platform_sleep_until_ns(int64_t time);

struct SDL_Surface;
typedef struct SDL_Surface *platform_image;
platform_image platform_create_image(uint32_t *data, int32_t width, int32_t height);
void platform_draw_image(platform_image image, int32_t x, int32_t y, int32_t width, int32_t height);

typedef int platform_random_access_file;
#define platform_CACHE_DAT 5
#define platform_CACHE_IDX(N) (N)
int platform_random_access_file_seek(platform_random_access_file file, int32_t pos);
int32_t platform_random_access_file_length(platform_random_access_file file);
struct platform_random_access_file_read_ctx {
    uint8_t *data;
    size_t len;
    size_t off;
};
static inline void platform_random_access_file_read_ctx_init(struct platform_random_access_file_read_ctx *self, uint8_t *data, int32_t len) {
    self->data = data;
    self->len = (size_t)len;
    self->off = 0;
}
int platform_random_access_file_read(platform_random_access_file file, struct platform_random_access_file_read_ctx *ctx);

