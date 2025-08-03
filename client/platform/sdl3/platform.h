#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define platform_MEMSET memset
#define platform_MEMMOVE memmove
#define platform_MEMCPY memcpy

int platform_frame_init(int32_t width, int32_t height);
void platform_print(char *text, ptrdiff_t text_len);
void platform_heap_reset(void *address);
void *platform_heap_alloc(size_t size, size_t align);

typedef int platform_random_access_file;
#define platform_CACHE_DAT 5
#define platform_CACHE_IDX(N) (N)
int platform_random_access_file_seek(platform_random_access_file file, int32_t pos);
int32_t platform_random_access_file_length(platform_random_access_file file);
int32_t platform_random_access_file_read(platform_random_access_file file, uint8_t *data, int32_t off, int32_t len);

