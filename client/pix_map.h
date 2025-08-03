struct pix_map {
    int32_t *data;
    int32_t width;
    int32_t height;
};

static void pix_map_bind(struct pix_map *self) {
    pix2d_bind(self->width, self->data, self->height);
}

static void pix_map_init(struct pix_map *self, int32_t width, int32_t height, int32_t *data) {
    self->width = width;
    self->height = height;
    self->data = data; // java: this.data = new int[height * width];

    pix_map_bind(self);
}
