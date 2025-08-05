#include "platform.h"
#include "file_stream.h"

static uint8_t file_stream_temp[520];

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

uint8_t *file_stream_read(struct file_stream *self, int32_t file, int32_t *out_length) {
    if (file_stream_seek(file * 6, self->idx) != 0) return NULL;

    int32_t n;
    for (int32_t off = 0; off < 6; off += n) {
        n = platform_random_access_file_read(self->idx, &file_stream_temp[0], off, 6 - off);
        if (n == -1) return NULL;
    }

    int32_t size = ((int32_t)file_stream_temp[0] << 16) + ((int32_t)file_stream_temp[1] << 8) + (int32_t)file_stream_temp[2];
    int32_t sector = ((int32_t)file_stream_temp[3] << 16) + ((int32_t)file_stream_temp[4] << 8) + (int32_t)file_stream_temp[5];

    if (size < 0 || size > self->max_file_size) return NULL;

    if (
        sector <= 0 ||
        sector > platform_random_access_file_length(self->dat) / 520
    ) return NULL;

    uint8_t *data = platform_heap_alloc(size, 1);
    int32_t pos = 0;
    int32_t part = 0;
    while (pos < size) {
        if (sector == 0) goto err_out;

        if (file_stream_seek(sector * 520, self->dat) != 0) goto err_out;

        int32_t off = 0;
        int32_t available = size - pos;
        if (available > 512) available = 512;

        while (off < available + 8) {
            int32_t read = platform_random_access_file_read(
                self->dat,
                &file_stream_temp[0],
                off,
                available + 8 - off
            );
            if (read == -1) goto err_out;

            off += read;
        }

        int32_t sector_file = ((int32_t)file_stream_temp[0] << 8) + (int32_t)file_stream_temp[1];
        int32_t sector_part = ((int32_t)file_stream_temp[2] << 8) + (int32_t)file_stream_temp[3];
        int32_t next_sector = ((int32_t)file_stream_temp[4] << 16) + ((int32_t)file_stream_temp[5] << 8) + (int32_t)file_stream_temp[6];
        int32_t sector_archive = file_stream_temp[7];

        if (
            file != sector_file ||
            part != sector_part ||
            self->archive != sector_archive
        ) goto err_out;

        if (
            next_sector < 0 ||
            next_sector > platform_random_access_file_length(self->dat) / 520
        ) goto err_out;

        for (int32_t i = 0; i < available; ++i) {
            data[pos++] = file_stream_temp[i + 8];
        }

        sector = next_sector;
        ++part;
    }

    *out_length = size;
    return data;

    err_out:
    platform_heap_reset(data);
    return NULL;
}
