#include "platform.h"
#include "jagfile.h"
#include "packet.h"
#include "bzip2.h"
#include "util.h"

// java: constructor and unpack()
// NOTE: `src` must be the latest allocation, and may be overwritten!
void jagfile_init(struct jagfile *self, uint8_t *src, ptrdiff_t src_length) {
    struct packet data;
    packet_init(&data, src, src_length);

    int32_t unpacked_size = packet_g3(&data);
    int32_t packed_size = packet_g3(&data);

    if (unpacked_size == packed_size) {
        self->data = src;
        self->unpacked = false;
    } else {
        uint8_t *temp = platform_heap_alloc(unpacked_size, 1);
        bzip2_decompress(&temp[0], unpacked_size, src, packed_size, 6);

        // NOTE: Added to make sure `src` isn't leaked.
        platform_MEMMOVE(&src[0], &temp[0], unpacked_size);
        platform_heap_reset(&src[unpacked_size]);
        temp = src;

        self->data = temp;

        packet_init(&data, self->data, unpacked_size);
        self->unpacked = true;
    }

    self->file_count = packet_g2(&data);

    int32_t *alloc = platform_heap_alloc(sizeof(int32_t) * self->file_count * 4, 4);
    self->file_hashes = alloc;
    alloc += self->file_count;
    self->file_unpacked_sizes = alloc;
    alloc += self->file_count;
    self->file_packed_sizes = alloc;
    alloc += self->file_count;
    self->file_offsets = alloc;

    int32_t pos = self->file_count * 10 + data.pos;
    for (int32_t i = 0; i < self->file_count; ++i) {
        self->file_hashes[i] = packet_g4(&data);
        self->file_unpacked_sizes[i] = packet_g3(&data);
        self->file_packed_sizes[i] = packet_g3(&data);
        self->file_offsets[i] = pos;
        pos += self->file_packed_sizes[i];
    }
}

// NOTE: Removed useless `dst` argument.
uint8_t *jagfile_read(struct jagfile *self, const char *name, ptrdiff_t name_length, ptrdiff_t *out_length) {
    int32_t hash = 0;
    for (ptrdiff_t i = 0; i < name_length; ++i) {
        int ch = name[i];
        hash = (int32_t)((uint32_t)hash * 61 + (uint32_t)util_TO_UPPER(ch) - 32);
    }

    for (int32_t i = 0; i < self->file_count; ++i) {
        if (self->file_hashes[i] == hash) {
            uint8_t *dst = platform_heap_alloc(self->file_unpacked_sizes[i], 1);

            if (self->unpacked) {
                platform_MEMCPY(&dst[0], &self->data[self->file_offsets[i]], self->file_unpacked_sizes[i]);
            } else {
                bzip2_decompress(
                    &dst[0],
                    self->file_unpacked_sizes[i],
                    &self->data[0],
                    self->file_packed_sizes[i],
                    self->file_offsets[i]
                );
            }

            *out_length = self->file_unpacked_sizes[i];
            return dst;
        }
    }

    return NULL;
}