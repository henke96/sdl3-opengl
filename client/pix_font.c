#include "platform.h"
#include "jagfile.h"
#include "pix_font.h"
#include "packet.h"
#include "pix2d.h"
#include "x.h"

static int pix_font_char_lookup[256];

// NOTE: Caller must add ".dat" suffix.
void pix_font_init(struct pix_font *self, struct jagfile *jagfile, const char *name, int32_t name_length) {
    // NOTE: Set fields to default values.
    self->strikethrough = false;
    self->height = 0;

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

    var5.pos = packet_g2(&var4) + 4;
    int32_t var7 = packet_g1(&var5);
    if (var7 > 0) var5.pos += (var7 - 1) * 3;

    for (int i = 0; i < 94; ++i) {
        self->char_offset_x[i] = packet_g1(&var5);
        self->char_offset_y[i] = packet_g1(&var5);

        int32_t var11 = self->char_mask_width[i] = packet_g2(&var5);
        int32_t var12 = self->char_mask_height[i] = packet_g2(&var5);
        int32_t var13 = packet_g1(&var5);
        int32_t var14;
        if (platform_CKD_MUL32(&var14, var11, var12)) platform_ABORT();
        self->char_mask[i] = platform_heap_alloc(var14, 1);
        if (var13 == 0) {
            for (int32_t j = 0; j < var14; ++j) {
                self->char_mask[i][j] = packet_g1b(&var4);
            }
        } else if (var13 == 1) {
            for (int32_t x = 0; x < var11; ++x) {
                for (int32_t y = 0; y < var12; ++y) {
                    self->char_mask[i][var11 * y + x] = packet_g1b(&var4);
                }
            }
        } else platform_ABORT();
        if (var12 > self->height) {
            self->height = var12;
        }

        self->char_offset_x[i] = 1;
        self->char_advance[i] = var11 + 2;

        int32_t var18 = 0;
        for (int32_t var19 = var12 / 7; var19 < var12; ++var19) {
            var18 += self->char_mask[i][var11 * var19];
        }
        // NOTE: Removed useless var10002.
        if (var18 <= var12 / 7) {
            --self->char_advance[i];
            self->char_offset_x[i] = 0;
        }
        int32_t var20 = 0;
        for (int32_t var21 = var12 / 7; var21 < var12; ++var21) {
            var20 += self->char_mask[i][var11 * var21 + (var11 - 1)];
        }
        if (var20 <= var12 / 7) {
            --self->char_advance[i];
        }
    }
    self->char_advance[94] = self->char_advance[8];
    for (int i = 0; i < 256; ++i) {
        self->draw_width[i] = self->char_advance[pix_font_char_lookup[i]];
    }

    // NOTE: Added to not leak `var4_data` and `var5_data`.
    ptrdiff_t len = (int8_t *)platform_heap_alloc(0, 1) - self->char_mask[0];
    platform_MEMMOVE(&var4_data[0], self->char_mask[0], len);
    ptrdiff_t diff = (int8_t *)&var4_data[0] - self->char_mask[0];
    for (int i = 0; i < 94; ++i) self->char_mask[i] += diff;
    platform_heap_reset(&var4_data[len]);
}

void pix_font_static_init(void) {
    // NOTE: Rewritten without string literal.

    uint8_t special[] = { 33 /* ! */, 34 /* " */, 163 /* pound */, 36 /* $ */, 37 /* % */, 94 /* ^ */, 38 /* & */, 42 /* * */, 40 /* ( */, 41 /* ) */, 45 /* - */, 95 /* _ */, 61 /* = */, 43 /* + */, 91 /* [ */, 123 /* { */, 93 /* ] */, 125 /* } */, 59 /* ; */, 58 /* : */, 39 /* ' */, 64 /* @ */, 35 /* # */, 126 /* ~ */, 44 /* , */, 60 /* < */, 46 /* . */, 62 /* > */, 47 /* / */, 63 /* ? */, 92 /* \ */, 124 /* | */, 32 /*   */};
    for (int i = 0; i < 256; ++i) {
        unsigned int off = (unsigned int)i - 65 /* A */;
        if (off <= 25 /* Z - A */) {
            pix_font_char_lookup[i] = off;
            continue;
        }
        off = (unsigned int)i - 97 /* a */;
        if (off <= 25 /* z - a */) {
            pix_font_char_lookup[i] = 26 + off;
            continue;
        }
        off = (unsigned int)i - 48 /* 0 */;
        if (off <= 9 /* 9 - 0 */) {
            pix_font_char_lookup[i] = 52 + off;
        }

        int special_off = 0;
        for (; special_off < (int)x_ARRAY_LEN(special); ++special_off) {
            if (i == special[special_off]) goto found_special;
        }
        pix_font_char_lookup[i] = 74;
        continue;

        found_special:
        pix_font_char_lookup[i] = 62 + special_off;
    }
}

int32_t pix_font_string_width(struct pix_font *self, uint8_t *arg0, int32_t arg0_length) {
    if (arg0 == NULL) return 0;

    int32_t width = 0;
    for (int32_t i = 0; i < arg0_length; ++i) {
        if (arg0[i] == 64 /* @ */ && i + 4 < arg0_length && arg0[i + 4] == 64 /* @ */) {
            i += 4;
        } else {
            width += self->draw_width[arg0[i]];
        }
    }
    return width;
}

static void pix_font_copy_pixels(int32_t *arg0, int8_t *arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5, int32_t arg6, int32_t arg7, int32_t arg8) {
    int32_t var10 = -(arg5 >> 2);
    int32_t var11 = -(arg5 & 0x3);
    for (int32_t var12 = -arg6; var12 < 0; ++var12) {
        // NOTE: Rewritten slightly.
        for (int32_t var13 = var10; var13 < 0; ++var13) {
            int32_t temp0 = arg1[arg3];
            if (temp0 != 0) arg0[arg4] = arg2;

            int32_t temp1 = arg1[arg3 + 1];
            if (temp1 != 0) arg0[arg4 + 1] = arg2;

            int32_t temp2 = arg1[arg3 + 2];
            if (temp2 != 0) arg0[arg4 + 2] = arg2;

            int32_t temp3 = arg1[arg3 + 3];
            if (temp3 != 0) arg0[arg4 + 3] = arg2;

            arg3 += 4;
            arg4 += 4;
        }
        for (int32_t var14 = var11; var14 < 0; ++var14) {
            int32_t temp = arg1[arg3++];
            if (temp != 0) arg0[arg4] = arg2;
            ++arg4;
        }

        arg4 += arg7;
        arg3 += arg8;
    }
}

static void pix_font_draw_char(int8_t *arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5) {
    int32_t var7 = pix2d_width * arg2 + arg1;
    int32_t var8 = pix2d_width - arg3;
    int32_t var9 = 0;
    int32_t var10 = 0;
    if (arg2 < pix2d_top) {
        int32_t var11 = pix2d_top - arg2;
        arg4 -= var11;
        arg2 = pix2d_top;
        var10 += arg3 * var11;
        var7 += pix2d_width * var11;
    }
    if (arg2 + arg4 >= pix2d_bottom) {
        arg4 -= arg2 + arg4 - pix2d_bottom + 1;
    }
    if (arg1 < pix2d_left) {
        int32_t var12 = pix2d_left - arg1;
        arg3 -= var12;
        arg1 = pix2d_left;
        var10 += var12;
        var7 += var12;
        var9 += var12;
        var8 += var12;
    }
    if (arg1 + arg3 >= pix2d_right) {
        int32_t var13 = arg1 + arg3 - pix2d_right + 1;
        arg3 -= var13;
        var9 += var13;
        var8 += var13;
    }
    if (arg3 > 0 && arg4 > 0) {
        pix_font_copy_pixels(&pix2d_data[0], arg0, arg5, var10, var7, arg3, arg4, var8, var9);
    }
}

void pix_font_draw_string(struct pix_font *self, uint8_t *arg0, int32_t arg0_length, int32_t arg1, int32_t arg3, int32_t arg4) {
    if (arg0 == NULL) return;

    int32_t var8 = arg3 - self->height;
    for (int32_t i = 0; i < arg0_length; ++i) {
        int var10 = pix_font_char_lookup[arg0[i]];
        if (var10 != 94) {
            pix_font_draw_char(
                self->char_mask[var10],
                self->char_offset_x[var10] + arg4,
                self->char_offset_y[var10] + var8,
                self->char_mask_width[var10],
                self->char_mask_height[var10],
                arg1
            );
        }
        arg4 += self->char_advance[var10];
    }
}