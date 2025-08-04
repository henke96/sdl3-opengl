#include "platform.h"
#include "jagfile.h"
#include "pix_font.h"
#include "packet.h"
#include "x.h"

static int pix_font_char_lookup[256];

// NOTE: Caller must add ".dat" suffix.
void pix_font_init(struct pix_font *self, struct jagfile *jagfile, const char *name, ptrdiff_t name_length) {
    // NOTE: Set fields to default values.
    self->strikethrough = false;
    self->height = 0;

    // TODO: Rename variables.
    struct packet var4;
    ptrdiff_t var4_data_length;
    uint8_t *var4_data = jagfile_read(jagfile, name, name_length, &var4_data_length);
    if (var4_data == NULL) platform_abort();
    packet_init(&var4, var4_data, var4_data_length);

    struct packet var5;
    ptrdiff_t var5_data_length;
    uint8_t *var5_data = jagfile_read(jagfile, x_STR_COMMA_LEN("index.dat"), &var5_data_length);
    if (var5_data == NULL) platform_abort();
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
        int32_t var14 = var11 * var12;
        self->char_mask[i] = platform_heap_alloc(var14, 1);
        if (var13 == 0) {
            for (int32_t j = 0; j < var14; ++j) {
                self->char_mask[i][j] = packet_g1b(&var4);
            }
        } else if (var13 == 1) {
            for (int32_t w = 0; w < var11; ++w) {
                for (int32_t h = 0; h < var12; ++h) {
                    self->char_mask[i][var11 * h + w] = packet_g1b(&var4);
                }
            }
        }
        if (var12 > self->height) {
            self->height = var12;
        }

        self->char_offset_x[i] = 1;
        self->char_advance[i] = var11 + 2;

        int32_t var18 = 0;
        for (int32_t h = var12 / 7; h < var12; ++h) {
            var18 += self->char_mask[i][var11 * h];
        }
        // NOTE: Removed useless var10002.
        if (var18 <= var12 / 7) {
            --self->char_advance[i];
            self->char_offset_x[i] = 0;
        }
        int32_t var20 = 0;
        for (int32_t h = var12 / 7; h < var12; ++h) {
            var20 += self->char_mask[i][var11 * h + (var11 - 1)];
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