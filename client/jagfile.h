struct jagfile {
    uint8_t *data;
    int32_t file_count;
    int32_t *file_hashes;
    int32_t *file_unpacked_sizes;
    int32_t *file_packed_sizes;
    int32_t *file_offsets;
    bool unpacked;
};

void jagfile_init(struct jagfile *self, uint8_t *src, ptrdiff_t src_length);
uint8_t *jagfile_read(struct jagfile *self, const char *name, ptrdiff_t name_length, ptrdiff_t *out_length); // NOTE: Removed useless `dst` argument.