#include "platform.h"
#include "jagfile.h"
#include "pix8.h"
#include "packet.h"
#include "x.h"

// NOTE: Caller must add ".dat" suffix.
void pix8_init(struct pix8 *self, struct jagfile *jagfile, const char *name, int32_t name_length, int32_t sprite) {
    // NOTE: Once we have the palette and pixel data it will be moved here.
    int32_t *palette_moved = platform_heap_alloc(0, 4);

    struct packet dat;
    int32_t dat_data_length;
    uint8_t *dat_data = jagfile_read(jagfile, name, name_length, &dat_data_length);
    if (dat_data == NULL) platform_ABORT(); // TODO: Should jagfile_read() abort instead?
    packet_init(&dat, dat_data, dat_data_length);

    struct packet idx;
    int32_t idx_data_length;
    uint8_t *idx_data = jagfile_read(jagfile, x_STR_COMMA_LEN("index.dat"), &idx_data_length);
    if (idx_data == NULL) platform_ABORT();
    packet_init(&idx, idx_data, idx_data_length);

    idx.pos = packet_g2(&dat);
    self->width = packet_g2(&idx);
    self->height = packet_g2(&idx);

    int32_t palette_len = packet_g1(&idx);
    self->palette = platform_heap_alloc(palette_len, 4);
    self->palette[0] = 0; // NOTE: Implicit in java.
    for (int32_t i = 0; i < palette_len - 1; ++i) {
        self->palette[i + 1] = packet_g3(&idx);
    }

    for (int32_t i = 0; i < sprite; ++i) {
        idx.pos += 2;
        int32_t len;
        if (platform_CKD_MUL32(&len, packet_g2(&idx), packet_g2(&idx))) platform_ABORT();
        dat.pos += len;
        ++idx.pos;
    }

    self->crop_right = packet_g1(&idx);
    self->crop_top = packet_g1(&idx);
    self->crop_right = packet_g2(&idx);
    self->crop_bottom = packet_g2(&idx);

    int32_t var9 = packet_g1(&idx);
    int32_t pixels_len;
    if (platform_CKD_MUL32(&pixels_len, self->crop_bottom, self->crop_right)) platform_ABORT();
    self->pixels = platform_heap_alloc(pixels_len, 4);
    if (var9 == 0) {
        for (int32_t i = 0; i < pixels_len; ++i) {
            self->pixels[i] = packet_g1b(&dat);
        }
    } else if (var9 == 1) {
        for (int32_t x = 0; x < self->crop_right; ++x) {
            for (int32_t y = 0; y < self->crop_bottom; ++y) {
                self->pixels[self->crop_right * y + x] = packet_g1b(&dat);
            }
        }
    } else platform_ABORT();

    // NOTE: Added to not leak `dat_data` and `idx_data`.
    platform_MEMMOVE(&palette_moved[0], &self->palette[0], palette_len * 4);
    self->palette = palette_moved;
    uint8_t *pixels_moved = (uint8_t *)&palette_moved[palette_len];
    platform_MEMMOVE(&pixels_moved[0], &self->pixels[0], pixels_len);
    self->pixels = pixels_moved;
    platform_heap_reset(&self->pixels[pixels_len]);
}