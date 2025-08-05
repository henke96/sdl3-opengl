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
