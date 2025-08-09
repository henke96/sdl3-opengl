struct pix8 {
    int32_t width;
    int32_t height;
    int32_t *palette;
    int32_t crop_left;
    int32_t crop_top;
    int32_t crop_right;
    int32_t crop_bottom;
    uint8_t *pixels;
};

// NOTE: Caller must add ".dat" suffix.
void pix8_init(struct pix8 *self, struct jagfile *jagfile, const char *name, int32_t name_length, int32_t sprite);
