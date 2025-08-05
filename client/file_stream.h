struct file_stream {
    platform_random_access_file dat;
    platform_random_access_file idx;
    int32_t archive;
    int32_t max_file_size;
};

void file_stream_init(
    struct file_stream *self,
    int32_t archive,
    platform_random_access_file idx,
    platform_random_access_file dat,
    int32_t max_file_size
);

uint8_t *file_stream_read(struct file_stream *self, int32_t file, int32_t *out_length);
