#include "platform.h"
#include "pix2d.h"
#include "pix_map.h"

void pix_map_init(struct pix_map *self, int32_t width, int32_t height, int32_t *data) {
    self->width = width;
    self->height = height;
    self->data = data; // java: this.data = new int[height * width];

    self->image = platform_create_image(self->data, width, height);

    pix_map_bind(self);
}

void pix_map_draw(struct pix_map *self, int32_t x, int32_t y) {
    platform_draw_image(self->image, x, y, self->width, self->height); // java: g.drawImage(this.image, x, y, this);
}
