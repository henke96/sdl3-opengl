#include "platform.h"
#include "jagfile.h"
#include "pix32.h"
#include "jpeg.h"
#include "pix2d.h"
#include "packet.h"
#include "x.h"

// NOTE: Caller must add ".dat" suffix.
void pix32_init_jagfile(struct pix32 *self, struct jagfile *jagfile, const char *name, int32_t name_length, int32_t arg2) {
    // NOTE: Once we have the pixel data it will be moved here.
    int32_t *pixels_moved = platform_heap_alloc(0, 4);

    // TODO: Rename variables.
    struct packet var4;
    int32_t var4_data_length;
    uint8_t *var4_data = jagfile_read(jagfile, name, name_length, &var4_data_length);
    if (var4_data == NULL) platform_ABORT();
    packet_init(&var4, var4_data, var4_data_length);

    struct packet var5;
    int32_t var5_data_length;
    uint8_t *var5_data = jagfile_read(jagfile, x_STR_COMMA_LEN("index.dat"), &var5_data_length);
    if (var5_data == NULL) platform_ABORT();
    packet_init(&var5, var5_data, var5_data_length);

    var5.pos = packet_g2(&var4);
    self->width = packet_g2(&var5);
    self->height = packet_g2(&var5);
    int32_t var6 = packet_g1(&var5);
    int32_t *var7 = platform_heap_alloc(var6, 4);
    var7[0] = 0; // NOTE: Implicit in java.
    for (int32_t i = 0; i < var6 - 1; ++i) {
        var7[i + 1] = packet_g3(&var5);
        if (var7[i + 1] == 0) {
            var7[i + 1] = 1;
        }
    }
    for (int32_t i = 0; i < arg2; ++i) {
        var5.pos += 2;
        var4.pos += packet_g2(&var5) * packet_g2(&var5);
        ++var5.pos;
    }
    self->crop_left = packet_g1(&var5);
    self->crop_top = packet_g1(&var5);
    self->crop_right = packet_g2(&var5);
    self->crop_bottom = packet_g2(&var5);
    int32_t var10 = packet_g1(&var5);
    int32_t len;
    if (platform_CKD_MUL32(&len, self->crop_bottom, self->crop_right)) platform_ABORT();
    self->pixels = platform_heap_alloc(len, 4);
    if (var10 == 0) {
        for (int32_t i = 0; i < len; ++i) {
            self->pixels[i] = var7[packet_g1(&var4)];
        }
    } else if (var10 == 1) {
        for (int32_t x = 0; x < self->crop_right; ++x) {
            for (int32_t y = 0; y < self->crop_bottom; ++y) {
                self->pixels[self->crop_right * y + x] = var7[packet_g1(&var4)];
            }
        }
    }

    // NOTE: Added to not leak `var4_data`, `var5_data` and `var7`.
    platform_MEMMOVE(&pixels_moved[0], &self->pixels[0], len * 4);
    self->pixels = pixels_moved;
    platform_heap_reset(&self->pixels[len]);
}

void pix32_init_jpeg(struct pix32 *self, uint8_t *src, int32_t src_length) {
    struct jpeg jpeg;
    self->pixels = jpeg_decode(&jpeg, &src[0], src_length, &self->crop_right, &self->crop_bottom);
    self->width = self->crop_right;
    self->height = self->crop_bottom;
    self->crop_left = 0;
    self->crop_top = 0;
}

static void pix32_copy_pixels(
    int32_t src_off,
    int32_t *src,
    int32_t arg2,
    int32_t arg4,
    int32_t dest_off,
    int32_t width,
    int32_t height,
    int32_t *dest
) {
    for (int32_t i = -height; i < 0; ++i) {
        // NOTE: Replaced loop with memmove.
        platform_MEMMOVE(&dest[dest_off], &src[src_off], width * 4);
        dest_off += width + arg4;
        src_off += width + arg2;
    }
}

// TODO: rename variables
void pix32_blit_opaque(struct pix32 *self, int32_t arg0, int32_t arg1) {
    int32_t var4 = self->crop_left + arg0;
    int32_t var5 = self->crop_top + arg1;
    int32_t var6 = pix2d_width * var5 + var4;
    int32_t var7 = 0;
    int32_t var8 = self->crop_bottom;
    int32_t var9 = self->crop_right;
    int32_t var10 = pix2d_width - var9;
    int32_t var11 = 0;
    if (var5 < pix2d_top) {
        int32_t var12 = pix2d_top - var5;
        var8 -= var12;
        var5 = pix2d_top;
        var7 += var9 * var12;
        var6 += pix2d_width * var12;
    }
    if (var5 + var8 > pix2d_bottom) {
        var8 -= var5 + var8 - pix2d_bottom;
    }
    if (var4 < pix2d_left) {
        int32_t var13 = pix2d_left - var4;
        var9 -= var13;
        var4 = pix2d_left;
        var7 += var13;
        var6 += var13;
        var11 += var13;
        var10 += var13;
    }
    if (var4 + var9 > pix2d_right) {
        int32_t var14 = var4 + var9 - pix2d_right;
        var9 -= var14;
        var11 += var14;
        var10 += var14;
    }
    if (var9 > 0 && var8 > 0) {
        pix32_copy_pixels(var7, self->pixels, var11, var10, var6, var9, var8, pix2d_data);
    }
}

static void pix32_copy_pixels2(int32_t *dest, int32_t *src, int32_t src_off, int32_t dest_off, int32_t arg5, int32_t arg6, int32_t arg7, int32_t arg8) {
    int32_t var10 = -(arg5 >> 2);
    int32_t var11 = -(arg5 & 0x3);
    for (int32_t i = -arg6; i < 0; ++i) {
        // NOTE: Rewritten slightly.
        for (int32_t j = var10; j < 0; ++j) {
            int32_t temp0 = src[src_off];
            if (temp0 != 0) dest[dest_off] = temp0;

            int32_t temp1 = src[src_off + 1];
            if (temp1 != 0) dest[dest_off + 1] = temp1;

            int32_t temp2 = src[src_off + 2];
            if (temp2 != 0) dest[dest_off + 2] = temp2;

            int32_t temp3 = src[src_off + 3];
            if (temp3 != 0) dest[dest_off + 3] = temp3;

            src_off += 4;
            dest_off += 4;
        }
        for (int j = var11; j < 0; ++j) {
            int32_t temp = src[src_off++];
            if (temp != 0) dest[dest_off] = temp;
            ++dest_off;
        }

        dest_off += arg7;
        src_off += arg8;
    }
}

void pix32_draw(struct pix32 *self, int32_t x, int32_t y) {
    x += self->crop_left;
    y += self->crop_top;

    int32_t dst_off = pix2d_width * y + x;
    int32_t src_off = 0;

    int32_t h = self->crop_bottom;
    int32_t w = self->crop_right;

    int32_t dst_step = pix2d_width - w;
    int32_t src_step = 0;

    if (y < pix2d_top) {
        int32_t cutoff = pix2d_top - y;
        h -= cutoff;
        src_off += w * cutoff;
        dst_off += pix2d_width * cutoff;
    }

    if (y + h > pix2d_bottom) {
        h -= y + h - pix2d_bottom;
    }

    if (x < pix2d_left) {
        int32_t cutoff = pix2d_left - x;
        w -= cutoff;
        x = pix2d_left;
        src_off += cutoff;
        dst_off += cutoff;
        src_step += cutoff;
        dst_step += cutoff;
    }

    if (x + w > pix2d_right) {
        int cutoff = x + w - pix2d_right;
        w -= cutoff;
        src_step += cutoff;
        dst_step += cutoff;
    }

    if (w > 0 && h > 0) {
        pix32_copy_pixels2(pix2d_data, self->pixels, src_off, dst_off, w, h, dst_step, src_step);
    }
}
