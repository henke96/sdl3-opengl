struct file_stream {
    platform_random_access_file dat;
    platform_random_access_file idx;
    int32_t archive;
    int32_t max_file_size;
};

struct file_stream_read_ctx {
    int32_t file;
    uint8_t *data;
    int32_t size;

    int32_t sector;
    uint8_t temp[520]; // TODO?
    union {
        struct {
            struct platform_random_access_file_read_ctx read_ctx;
        } s1;
        struct {
            int32_t pos;
            int32_t part;
            int32_t available;
            struct platform_random_access_file_read_ctx read_ctx;
        } s2;
    } s;
    int state;
};

static inline void file_stream_read_ctx_init(struct file_stream_read_ctx *self, int32_t file) {
    self->file = file;
    self->state = 0;
}

void file_stream_init(
    struct file_stream *self,
    int32_t archive,
    platform_random_access_file idx,
    platform_random_access_file dat,
    int32_t max_file_size
);

int file_stream_read(struct file_stream *self, struct file_stream_read_ctx *ctx);
