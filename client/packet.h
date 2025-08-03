struct packet { // TODO: rename? TODO: extends DoublyLinkable
    uint8_t *data;
    ptrdiff_t data_length;
    int32_t pos;
    // TODO: bit_pos
};

static inline void packet_init(struct packet *self, uint8_t *src, ptrdiff_t src_length) {
    self->data = src;
    self->data_length = src_length;
    self->pos = 0;
}

static inline int32_t packet_g1(struct packet *self) {
    return self->data[self->pos++];
}

static inline int8_t packet_g1b(struct packet *self) {
    return (int8_t)self->data[self->pos++];
}

static inline int32_t packet_g2(struct packet *self) {
    self->pos += 2;
    return (self->data[self->pos - 2] << 8) + self->data[self->pos - 1];
}

static inline int32_t packet_g3(struct packet *self) {
    self->pos += 3;
    return (self->data[self->pos - 3] << 16) + (self->data[self->pos - 2] << 8) + self->data[self->pos - 1];
}

static inline int32_t packet_g4(struct packet *self) {
    self->pos += 4;
    return (int32_t)((uint32_t)self->data[self->pos - 4] << 24) + (self->data[self->pos - 3] << 16) + (self->data[self->pos - 2] << 8) + self->data[self->pos - 1];
}
