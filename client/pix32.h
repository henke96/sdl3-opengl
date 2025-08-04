struct pix32 { // TODO: extends Pix2D
    int32_t *pixels;
    int32_t width;
    int32_t height;
    int32_t crop_right;
    int32_t crop_bottom;
    int32_t crop_top;
    int32_t crop_left;
};

// NOTE: Caller must add ".dat" suffix.
void pix32_init_jagfile(struct pix32 *self, struct jagfile *jagfile, const char *name, ptrdiff_t name_length, int32_t arg2);
void pix32_init_jpeg(struct pix32 *self, uint8_t *jpeg, ptrdiff_t jpeg_length);
void pix32_blit_opaque(struct pix32 *self, int32_t arg0, int32_t arg1);
void pix32_draw(struct pix32 *self, int32_t x, int32_t y);
