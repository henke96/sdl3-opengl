#include "platform.h"
#include "pix2d.h"

int32_t *pix2d_data;
int32_t pix2d_width;
int32_t pix2d_height;
int32_t pix2d_top;
int32_t pix2d_bottom;
int32_t pix2d_left;
int32_t pix2d_right;
int32_t pix2d_safe_width;
int32_t pix2d_center_x;
int32_t pix2d_center_y;

void pix2d_bind(int32_t width, int32_t *data, int32_t height) {
    pix2d_data = data;
    pix2d_width = width;
    pix2d_height = height;
    pix2d_set_bounds(width, height, 0, 0);
}

void pix2d_reset_bounds(void) {
    pix2d_left = 0;
    pix2d_top = 0;
    pix2d_right = pix2d_width;
    pix2d_bottom = pix2d_height;
    pix2d_safe_width = pix2d_right - 1;
    pix2d_center_x = pix2d_right / 2;
}

void pix2d_set_bounds(int32_t right, int32_t bottom, int32_t top, int32_t left) {
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > pix2d_width) right = pix2d_width;
    if (bottom > pix2d_height) bottom = pix2d_height;

    pix2d_left = left;
    pix2d_top = top;
    pix2d_right = right;
    pix2d_bottom = bottom;
    pix2d_safe_width = pix2d_right - 1;
    pix2d_center_x = pix2d_right / 2;
    pix2d_center_y = pix2d_bottom / 2;
}

void pix2d_clear(void) {
    int32_t size = pix2d_height * pix2d_width;
    platform_MEMSET(pix2d_data, 0, size);
}

void pix2d_fill_rect_trans(int32_t y, int32_t alpha, int32_t height, int32_t width, int32_t colour, int32_t x) {
    if (x < pix2d_left) {
        width -= pix2d_left - x;
        x = pix2d_left;
    }

    if (y < pix2d_top) {
        height -= pix2d_top - y;
        y = pix2d_top;
    }

    if (width + x > pix2d_right) {
        width = pix2d_right - x;
    }

    if (y + height > pix2d_bottom) {
        height = pix2d_bottom - y;
    }

    int32_t inv_alpha = 256 - alpha;
    int32_t r0 = (colour >> 16 & 0xFF) * alpha;
    int32_t g0 = (colour >> 8 & 0xFF) * alpha;
    int32_t b0 = (colour & 0xFF) * alpha;
    int32_t step = pix2d_width - width;
    int32_t offset = pix2d_width * y + x;

    for (int32_t i = 0; i < height; i++) {
        for (int32_t j = -width; j < 0; j++) {
            int32_t r1 = (pix2d_data[offset] >> 16 & 0xFF) * inv_alpha;
            int32_t g1 = (pix2d_data[offset] >> 8 & 0xFF) * inv_alpha;
            int32_t b1 = (pix2d_data[offset] & 0xFF) * inv_alpha;

            int32_t rgb = ((b0 + b1) >> 8) + ((r0 + r1) >> 8 << 16) + ((g0 + g1) >> 8 << 8);
            pix2d_data[offset++] = rgb;
        }

        offset += step;
    }
}

void pix2d_fill_rect(int32_t colour, int32_t width, int32_t height, int32_t x, int32_t y) {
    if (x < pix2d_left) {
        width -= pix2d_left - x;
        x = pix2d_left;
    }

    if (y < pix2d_top) {
        height -= pix2d_top - y;
        y = pix2d_top;
    }

    if (width + x > pix2d_right) {
        width = pix2d_right - x;
    }

    if (height + y > pix2d_bottom) {
        height = pix2d_bottom - y;
    }

    int32_t step = pix2d_width - width;
    int32_t offset = pix2d_width * y + x;

    for (int32_t i = -height; i < 0; i++) {
        for (int32_t j = -width; j < 0; j++) {
            pix2d_data[offset++] = colour;
        }

        offset += step;
    }
}

// TODO: put in header for inlining?
void pix2d_draw_rect(int32_t height, int32_t width, int32_t colour, int32_t x, int32_t y) {
    pix2d_draw_horisontal_line(colour, y, width, x);
    pix2d_draw_horisontal_line(colour, height + y - 1, width, x);
    pix2d_draw_vertical_line(x, colour, y, height);
    pix2d_draw_vertical_line(width + x - 1, colour, y, height);
}

void pix2d_draw_rect_trans(int height, int colour, int x, int y, int width, int alpha) {
    pix2d_draw_horisontal_line_trans(y, width, colour, x, alpha);
    pix2d_draw_horisontal_line_trans(height + y - 1, width, colour, x, alpha);
    if (height >= 3) {
        pix2d_draw_vertical_line_trans(x, y + 1, alpha, height - 2, colour);
        pix2d_draw_vertical_line_trans(x + width - 1, y + 1, alpha, height - 2, colour);
    }
}

void pix2d_draw_horisontal_line(int32_t colour, int32_t y, int32_t width, int32_t x) {
    if (y < pix2d_top || y >= pix2d_bottom) return;

    if (x < pix2d_left) {
        width -= pix2d_left - x;
        x = pix2d_left;
    }

    if (width + x > pix2d_right) {
        width = pix2d_right - x;
    }

    int32_t offset = pix2d_width * y + x;

    for (int32_t i = 0; i < width; i++) {
        pix2d_data[offset + i] = colour;
    }
}

void pix2d_draw_horisontal_line_trans(int32_t y, int32_t width, int32_t colour, int32_t x, int32_t alpha) {
    if (y < pix2d_top || y >= pix2d_bottom) return;

    if (x < pix2d_left) {
        width -= pix2d_left - x;
        x = pix2d_left;
    }

    if (width + x > pix2d_right) {
        width = pix2d_right - x;
    }

    int32_t inv_alpha = 256 - alpha;
    int32_t r0 = (colour >> 16 & 0xFF) * alpha;
    int32_t g0 = (colour >> 8 & 0xFF) * alpha;
    int32_t b0 = (colour & 0xFF) * alpha;
    int32_t offset = pix2d_width * y + x;

    for (int32_t i = 0; i < width; i++) {
        int32_t r1 = (pix2d_data[offset] >> 16 & 0xFF) * inv_alpha;
        int32_t g1 = (pix2d_data[offset] >> 8 & 0xFF) * inv_alpha;
        int32_t b1 = (pix2d_data[offset] & 0xFF) * inv_alpha;

        int32_t rgb = ((b0 + b1) >> 8) + ((r0 + r1) >> 8 << 16) + ((g0 + g1) >> 8 << 8);
        pix2d_data[offset++] = rgb;
    }
}

void pix2d_draw_vertical_line(int32_t x, int32_t colour, int32_t y, int32_t height) {
    if (x < pix2d_left || x >= pix2d_right) return;

    if (y < pix2d_top) {
        height -= pix2d_top - y;
        y = pix2d_top;
    }

    if (y + height > pix2d_bottom) {
        height = pix2d_bottom - y;
    }

    int32_t offset = pix2d_width * y + x;

    for (int32_t i = 0; i < height; i++) {
        pix2d_data[pix2d_width * i + offset] = colour;
    }
}

void pix2d_draw_vertical_line_trans(int32_t x, int32_t y, int32_t alpha, int32_t height, int32_t colour) {
    if (x < pix2d_left || x >= pix2d_right) return;

    if (y < pix2d_top) {
        height -= pix2d_top - y;
        y = pix2d_top;
    }

    if (y + height > pix2d_bottom) {
        height = pix2d_bottom - y;
    }

    int32_t inv_alpha = 256 - alpha;
    int32_t r0 = (colour >> 16 & 0xFF) * alpha;
    int32_t g0 = (colour >> 8 & 0xFF) * alpha;
    int32_t b0 = (colour & 0xFF) * alpha;
    int32_t offset = pix2d_width * y + x;

    for (int32_t i = 0; i < height; i++) {
        int32_t r1 = (pix2d_data[offset] >> 16 & 0xFF) * inv_alpha;
        int32_t g1 = (pix2d_data[offset] >> 8 & 0xFF) * inv_alpha;
        int32_t b1 = (pix2d_data[offset] & 0xFF) * inv_alpha;

        int32_t rgb = ((b0 + b1) >> 8) + ((r0 + r1) >> 8 << 16) + ((g0 + g1) >> 8 << 8);
        pix2d_data[offset] = rgb;

        offset += pix2d_width;
    }
}
