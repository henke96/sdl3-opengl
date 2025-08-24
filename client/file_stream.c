#include "platform.h"
#include "file_stream.h"

void file_stream_init(
    struct file_stream *self,
    int32_t archive,
    platform_random_access_file idx,
    platform_random_access_file dat,
    int32_t max_file_size
) {
    self->archive = archive;
    self->dat = dat;
    self->idx = idx;
    self->max_file_size = max_file_size;
}

static int file_stream_seek(int32_t pos, platform_random_access_file file) {
    if (pos < 0 || pos > 0x3c00000) {
        // TODO: System.out.println("Badseek - pos:" + pos + " len:" + file.length());
        pos = 0x3c00000;

        // TODO:
        // try {
        //     Thread.sleep(1000L);
        // } catch (Exception ignore) {}
    }

    return platform_random_access_file_seek(file, pos);
}

int file_stream_read(struct file_stream *self, struct file_stream_read_ctx *ctx) {
    int status;

    switch (ctx->state) {
        case 0:
        ctx->state = 1;

        if (file_stream_seek(ctx->file * 6, self->idx) != 0) return -1;
        platform_random_access_file_read_ctx_init(&ctx->s.s1.read_ctx, &ctx->temp[0], 6);

        case 1:
        status = platform_random_access_file_read(self->idx, &ctx->s.s1.read_ctx);
        if (status <= 0) return status;
        ctx->state = 2;

        ctx->size = ((int32_t)ctx->temp[0] << 16) + ((int32_t)ctx->temp[1] << 8) + (int32_t)ctx->temp[2];
        ctx->sector = ((int32_t)ctx->temp[3] << 16) + ((int32_t)ctx->temp[4] << 8) + (int32_t)ctx->temp[5];

        if (ctx->size < 0 || ctx->size > self->max_file_size) return -1;

        if (
            ctx->sector <= 0 ||
            ctx->sector > platform_random_access_file_length(self->dat) / 520
        ) return -1;

        ctx->data = platform_heap_alloc(ctx->size, 1);

        ctx->s.s2.pos = 0;
        ctx->s.s2.part = 0;
        while (ctx->s.s2.pos < ctx->size) {
            if (ctx->sector == 0) goto err_out;

            if (file_stream_seek(ctx->sector * 520, self->dat) != 0) goto err_out;

            ctx->s.s2.available = ctx->size - ctx->s.s2.pos;
            if (ctx->s.s2.available > 512) ctx->s.s2.available = 512;
            platform_random_access_file_read_ctx_init(&ctx->s.s2.read_ctx, &ctx->temp[0], ctx->s.s2.available + 8);
        
            case 2:
            status = platform_random_access_file_read(self->dat, &ctx->s.s2.read_ctx);
            if (status <= 0) return status;

            int32_t sector_file = ((int32_t)ctx->temp[0] << 8) + (int32_t)ctx->temp[1];
            int32_t sector_part = ((int32_t)ctx->temp[2] << 8) + (int32_t)ctx->temp[3];
            int32_t next_sector = ((int32_t)ctx->temp[4] << 16) + ((int32_t)ctx->temp[5] << 8) + (int32_t)ctx->temp[6];
            int32_t sector_archive = ctx->temp[7];

            if (
                ctx->file != sector_file ||
                ctx->s.s2.part != sector_part ||
                self->archive != sector_archive
            ) goto err_out;

            if (
                next_sector < 0 ||
                next_sector > platform_random_access_file_length(self->dat) / 520
            ) goto err_out;

            for (int32_t i = 0; i < ctx->s.s2.available; ++i) {
                ctx->data[ctx->s.s2.pos++] = ctx->temp[i + 8];
            }

            ctx->sector = next_sector;
            ++ctx->s.s2.part;
        }
        return 1;

        default: platform_UNREACHABLE();
    }
    err_out:
    platform_heap_reset(ctx->data);
    return -1;
}
