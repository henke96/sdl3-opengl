struct pix_font {
    int32_t char_mask_width[94];
    int32_t char_mask_height[94];
    int32_t char_offset_x[94];
    int32_t char_offset_y[94];
    int32_t char_advance[95];
    int32_t draw_width[256];
    int32_t height;
    int8_t *char_mask[94];
    bool strikethrough;
    // TODO: public Random random = new Random();
};

void pix_font_static_init(void);
void pix_font_init(struct pix_font *self, struct jagfile *jagfile, const char *name, int32_t name_length);

// TODO: rename variables
int32_t pix_font_string_width(struct pix_font *self, uint8_t *arg0, int32_t arg0_length);
void pix_font_draw_string(struct pix_font *self, uint8_t *arg0, int32_t arg0_length, int32_t arg1, int32_t arg3, int32_t arg4);
static inline void pix_font_draw_string_center(struct pix_font *self, int32_t arg0, int32_t arg2, uint8_t *arg3, int32_t arg3_length, int32_t arg4) {
    pix_font_draw_string(self, arg3, arg3_length, arg2, arg4, arg0 - pix_font_string_width(self, arg3, arg3_length) / 2);
}
