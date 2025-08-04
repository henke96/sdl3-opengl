struct pix_map {
    int32_t *data;
    int32_t width;
    int32_t height;
    platform_image image;
};

static inline void pix_map_bind(struct pix_map *self) {
    pix2d_bind(self->width, self->data, self->height);
}

void pix_map_init(struct pix_map *self, int32_t width, int32_t height, int32_t *data);
void pix_map_draw(struct pix_map *self, int32_t x, int32_t y);

