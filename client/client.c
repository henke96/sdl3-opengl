#include "platform.h"
#include "client.h"
#include "x.h"
#include "util.h"
#include "pix2d.h"
#include "pix3d.h"
#include "pix_map.h"
#include "world.h"
#include "world3d.h"
#include "jagfile.h"
#include "file_stream.h"
#include "pix_font.h"
#include "pix32.h"
#include "pix8.h"

struct client_gameshell {
    struct pix_map draw_area;
    int32_t screen_width;
    int32_t screen_height;
};

struct client {
    struct client_gameshell gameshell;
    union {
        struct {
            int32_t image_title0_data[128 * 265];
            struct pix_map image_title0;
            int32_t image_title1_data[128 * 265];
            struct pix_map image_title1;
            int32_t image_title2_data[509 * 171];
            struct pix_map image_title2;
            int32_t image_title3_data[360 * 132];
            struct pix_map image_title3;
            int32_t image_title4_data[360 * 200];
            struct pix_map image_title4;
            int32_t image_title5_data[202 * 238];
            struct pix_map image_title5;
            int32_t image_title6_data[203 * 238];
            struct pix_map image_title6;
            int32_t image_title7_data[74 * 94];
            struct pix_map image_title7;
            int32_t image_title8_data[75 * 94];
            struct pix_map image_title8;
            int32_t image_flames_left_data[128 * 265];
            struct pix32 image_flames_left;
            int32_t image_flames_right_data[128 * 265];
            struct pix32 image_flames_right;
            int32_t flame_gradient0[256];
            int32_t flame_gradient1[256];
            int32_t flame_gradient2[256];
            int32_t flame_gradient[256];
            int32_t flame_buffer0[32768];
            int32_t flame_buffer1[32768];
            int32_t flame_buffer2[32768];
            int32_t flame_buffer3[32768];
        } title;
        struct {

        } game;
    } s;
    struct pix8 image_titlebox;
    struct pix8 image_titlebutton;
    struct pix8 image_redstone1;
    struct pix8 image_redstone2;
    struct pix8 image_redstone3;
    struct pix8 image_redstone1h;
    struct pix8 image_redstone2h;
    struct pix8 image_redstone1v;
    struct pix8 image_redstone2v;
    struct pix8 image_redstone3v;
    struct pix8 image_redstone1hv;
    struct pix8 image_redstone2hv;
    struct pix8 image_backbase1;
    struct pix8 image_backbase2;
    struct pix8 image_backhmid1;
    struct pix8 image_invback;
    struct pix8 image_chatback;
    struct pix8 image_runes[12];
    struct pix_font font_plain_11;
    struct pix_font font_plain_12;
    struct pix_font font_bold_12;
    struct pix_font font_quill_8;
    struct jagfile jag_title;
    struct file_stream file_streams[5];
    int32_t jag_checksum[9];
    bool flame_active;
    bool redraw_frame;
};

static struct client client;

static int32_t client_node_id;
int32_t client_port_offset;
static bool client_members_world;
static bool client_low_memory;

static void client_set_low_memory(void) {
    world3d_low_memory = true;
    pix3d_low_memory = true;
    client_low_memory = true;
    world_low_memory = true;
}

static void client_set_high_memory(void) {
    world3d_low_memory = false;
    pix3d_low_memory = false;
    client_low_memory = false;
    world_low_memory = false;
}

static void client_load_title(void) {
    // TODO?
    // if (this.imageTitle2 != null) {
    //     return;
    // } 
    // super.drawArea = null;
    // this.areaChatback = null;
    // this.areaMapback = null;
    // this.areaSidebar = null;
    // this.areaViewport = null;
    // this.areaBackbase1 = null;
    // this.areaBackbase2 = null;
    // this.areaBackhmid1 = null;

    pix_map_init(&client.s.title.image_title0, 128, 265, &client.s.title.image_title0_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title1, 128, 265, &client.s.title.image_title1_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title2, 509, 171, &client.s.title.image_title2_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title3, 360, 132, &client.s.title.image_title3_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title4, 360, 200, &client.s.title.image_title4_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title5, 202, 238, &client.s.title.image_title5_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title6, 203, 238, &client.s.title.image_title6_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title7, 74, 94, &client.s.title.image_title7_data[0]);
    pix2d_clear();

    pix_map_init(&client.s.title.image_title8, 75, 94, &client.s.title.image_title8_data[0]);
    pix2d_clear();

    // TODO:
    // if (this.jagTitle != null) {
    //     this.loadTitleBackground();
    //     this.loadTitleImages();
    // }

    client.redraw_frame = true;
}

static void client_draw_progress(int32_t percent, uint8_t *message, int32_t message_len) {
    // TODO:
    // this.lastProgressPercent = percent;
    // this.lastProgressMessage = message;

    // NOTE: client_load_title() moved out of this function. TODO?
    // TODO: 
    // if (this.jagTitle == null) {
    //     super.drawProgress(percent, message);
    //     return;
    // }

    pix_map_bind(&client.s.title.image_title4);

    int32_t x = 360;
    int32_t y = 200;

    int32_t offset_y = 20;
    // java: "RuneScape is loading - please wait..."
    uint8_t string[] = {
        82, 117,  110, 101, 83, 99, 97, 112, 101,
        32,
        105, 115,
        32,
        108, 111, 97, 100, 105, 110, 103,
        32,
        45,
        32,
        112, 108, 101, 97, 115, 101,
        32,
        119, 97, 105, 116, 46, 46, 46
    };
    pix_font_draw_string_center(
        &client.font_bold_12,
        x / 2,
        0xFFFFFF,
        &string[0], sizeof(string),
        y / 2 - 26 - offset_y
    );

    int32_t mid_y = y / 2 - 18 - offset_y;
    pix2d_draw_rect(34, 304, 0x8C1111, x / 2 - 152, mid_y);
    pix2d_draw_rect(32, 302, 0, x / 2 - 151, mid_y + 1);
    pix2d_fill_rect(0x8C1111, percent * 3, 30, x / 2 - 150, mid_y + 2);
    pix2d_fill_rect(0, 300 - percent * 3, 30, percent * 3 + (x / 2 - 150), mid_y + 2);
    pix_font_draw_string_center(&client.font_bold_12, x / 2, 0xFFFFFF, message, message_len, y / 2 + 5 - offset_y);

    pix_map_draw(&client.s.title.image_title4, 202, 171);

    if (client.redraw_frame) {
        client.redraw_frame = false;

        if (!client.flame_active) {
            pix_map_draw(&client.s.title.image_title0, 0, 0);
            pix_map_draw(&client.s.title.image_title1, 637, 0);
        }

        pix_map_draw(&client.s.title.image_title2, 128, 0);
        pix_map_draw(&client.s.title.image_title3, 202, 371);
        pix_map_draw(&client.s.title.image_title5, 0, 265);
        pix_map_draw(&client.s.title.image_title6, 562, 265);
        pix_map_draw(&client.s.title.image_title7, 128, 171);
        pix_map_draw(&client.s.title.image_title8, 562, 171);
    }
}

static void client_get_jag_file(int32_t crc, char *name, int32_t file, char *display_name, struct jagfile *out_file) {
    uint8_t *data = NULL;
    int32_t retry = 5;

    // TODO: if (this.fileStreams[0] != null) {
    int32_t data_length;
    data = file_stream_read(&client.file_streams[0], file, &data_length);

    if (data != NULL) {
        int32_t checksum = util_crc32(data, data_length);
        if (crc != checksum) data = NULL;
    }

    if (data != NULL) {
        jagfile_init(out_file, data, data_length);
        return;
    }

    // TODO: Download loop..
    for (;;);
}

static void client_load_title_background(void) {
    int32_t src_length;
    uint8_t *src = jagfile_read(&client.jag_title, x_STR_COMMA_LEN("title.dat"), &src_length);

    struct pix32 background;
    pix32_init_jpeg(&background, src, src_length);

    pix_map_bind(&client.s.title.image_title0);
    pix32_blit_opaque(&background, 0, 0);

    pix_map_bind(&client.s.title.image_title1);
    pix32_blit_opaque(&background, -637, 0);

    pix_map_bind(&client.s.title.image_title2);
    pix32_blit_opaque(&background, -128, 0);

    pix_map_bind(&client.s.title.image_title3);
    pix32_blit_opaque(&background, -202, -371);

    pix_map_bind(&client.s.title.image_title4);
    pix32_blit_opaque(&background, -202, -171);

    pix_map_bind(&client.s.title.image_title5);
    pix32_blit_opaque(&background, 0, -265);

    pix_map_bind(&client.s.title.image_title6);
    pix32_blit_opaque(&background, -562, -265);

    pix_map_bind(&client.s.title.image_title7);
    pix32_blit_opaque(&background, -128, -171);

    pix_map_bind(&client.s.title.image_title8);
    pix32_blit_opaque(&background, -562, -171);

    // draw right side (mirror image)
    int32_t *pixels = platform_heap_alloc(background.crop_right, 4);
    for (int32_t y = 0; y < background.crop_bottom; ++y) {
        for (int32_t x = 0; x < background.crop_right; ++x) {
            pixels[x] = background.pixels[background.crop_right * y + background.crop_right - x - 1];
        }

        for (int32_t x = 0; x < background.crop_right; ++x) {
            background.pixels[background.crop_right * y + x] = pixels[x];
        }
    }

    pix_map_bind(&client.s.title.image_title0);
    pix32_blit_opaque(&background, 382, 0);

    pix_map_bind(&client.s.title.image_title1);
    pix32_blit_opaque(&background, -255, 0);

    pix_map_bind(&client.s.title.image_title2);
    pix32_blit_opaque(&background, 254, 0);

    pix_map_bind(&client.s.title.image_title3);
    pix32_blit_opaque(&background, 180, -371);

    pix_map_bind(&client.s.title.image_title4);
    pix32_blit_opaque(&background, 180, -171);

    pix_map_bind(&client.s.title.image_title5);
    pix32_blit_opaque(&background, 382, -265);

    pix_map_bind(&client.s.title.image_title6);
    pix32_blit_opaque(&background, -180, -265);

    pix_map_bind(&client.s.title.image_title7);
    pix32_blit_opaque(&background, 254, -171);

    pix_map_bind(&client.s.title.image_title8);
    pix32_blit_opaque(&background, -180, -171);

    struct pix32 logo;
    // NOTE: Adding ".dat" suffix here instead of inside of `pix32_init_jagfile`.
    pix32_init_jagfile(&logo, &client.jag_title, x_STR_COMMA_LEN("logo.dat"), 0);
    pix_map_bind(&client.s.title.image_title2);
    pix32_draw(&logo, 382 - logo.crop_right / 2 - 128, 18);

    platform_heap_reset(src);
}

static void client_update_flame_buffer(struct pix8 *image) {
    int32_t height = 256;

    platform_MEMSET(&client.s.title.flame_buffer0[0], 0, sizeof(client.s.title.flame_buffer0));

    for (int i = 0; i < 5000; ++i) {
        int32_t rand = platform_random(128 * height);
        client.s.title.flame_buffer0[rand] = platform_random(256);
    }

    int32_t *flame_buffer0 = &client.s.title.flame_buffer0[0];
    int32_t *flame_buffer1 = &client.s.title.flame_buffer1[0];
    for (int i = 0; i < 20; ++i) {
        for (int32_t y = 1; y < height - 1; ++y) {
            for (int32_t x = 1; x < 127; ++x) {
                int32_t index = (y << 7) + x;
                flame_buffer1[index] = (
                    flame_buffer0[index - 1] +
                    flame_buffer0[index + 1] + 
                    flame_buffer0[index - 128] +
                    flame_buffer0[index + 128]
                ) / 4;
            }
        }

        int32_t *last = flame_buffer0;
        flame_buffer0 = flame_buffer1;
        flame_buffer1 = last;
    }

    if (image != NULL) {
        int32_t off = 0;

        for (int32_t y = 0; y < image->crop_bottom; ++y) {
            for (int32_t x = 0; x < image->crop_right; ++x) {
                if (image->pixels[off++] != 0) {
                    int32_t x0 = x + 16 + image->crop_left;
                    int32_t y0 = y + 16 + image->crop_top;
                    int32_t index = (y0 << 7) + x0;

                    client.s.title.flame_buffer0[index] = 0;
                }
            }
        }
    }
}

static void client_load_title_images(void) {
    // NOTE: titlebox, titlebutton and runes loading moved out of this function.

    pix32_init(&client.s.title.image_flames_left, 128, 265, &client.s.title.image_flames_left_data[0]);
    pix32_init(&client.s.title.image_flames_right, 128, 265, &client.s.title.image_flames_right_data[0]);
    platform_MEMCPY(&client.s.title.image_flames_left.pixels[0], &client.s.title.image_title0.data[0], 128 * 265 * 4);
    platform_MEMCPY(&client.s.title.image_flames_right.pixels[0], &client.s.title.image_title1.data[0], 128 * 265 * 4);

    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient0[i] = i * 262144;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient0[i + 64] = i * 1024 + 16711680;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient0[i + 128] = i * 4 + 16776960;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient0[i + 192] = 16777215;
    }

    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient1[i] = i * 1024;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient1[i + 64] = i * 4 + 65280;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient1[i + 128] = i * 262144 + 65535;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient1[i + 192] = 16777215;
    }

    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient2[i] = i * 4;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient2[i + 64] = i * 262144 + 255;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient2[i + 128] = i * 1024 + 16711935;
    }
    for (int32_t i = 0; i < 64; ++i) {
        client.s.title.flame_gradient2[i + 192] = 16777215;
    }

    // NOTE: `flame_buffer0` is zeroed in `client_update_flame_buffer`.
    platform_MEMSET(&client.s.title.flame_buffer1[0], 0, sizeof(client.s.title.flame_buffer1));
    client_update_flame_buffer(NULL);
    // TODO: zero flame buffer 2 and 3?

    // java: "Connecting to fileserver"
    uint8_t message[] = {
        67, 111, 110, 110, 101, 99, 116, 105, 110, 103,
        32,
        116, 111,
        32,
        102, 105, 108, 101, 115, 101, 114, 118, 101, 114
    };
    client_draw_progress(10, &message[0], sizeof(message));
}

// java: load() up until TODO
static void client_load0(void) {
    // NOTE: Skipping mindel, errorStarted, and errorHost logic.

    // TODO: if (SignLink.cache_dat != null) {
    for (int i = 0; i < 5; ++i) {
        file_stream_init(
            &client.file_streams[i],
            i + 1,
            platform_CACHE_IDX(i), // java: SignLink.cache_idx[i]
            platform_CACHE_DAT, // java: SignLink.cache_dat
            500000
        );
    }

    // NOTE: Hardcoding cache checksums instead of fetching them over HTTP.
    client.jag_checksum[0] = 0;
    client.jag_checksum[1] = 126707642; // TODO: 2004scape: -945108033
    client.jag_checksum[2] = 1573679574; // TODO: 2004scape: 1219858706
    client.jag_checksum[3] = 2074207176; // TODO: 2004scape: -1064880969
    client.jag_checksum[4] = -151945349; // TODO: 2004scape: 1633496510
    client.jag_checksum[5] = -390182005; // TODO: 2004scape: -171619975
    client.jag_checksum[6] = 245278618; // TODO: 2004scape: 399321136
    client.jag_checksum[7] = -87627495;
    client.jag_checksum[8] = -855112082;

    client_get_jag_file(client.jag_checksum[1], "title", 1, "title screen", &client.jag_title);
    // NOTE: Adding ".dat" suffix here instead of inside of `pix_font_init`.
    pix_font_init(&client.font_plain_11, &client.jag_title, x_STR_COMMA_LEN("p11.dat"));
    pix_font_init(&client.font_plain_12, &client.jag_title, x_STR_COMMA_LEN("p12.dat"));
    pix_font_init(&client.font_bold_12, &client.jag_title, x_STR_COMMA_LEN("b12.dat"));
    pix_font_init(&client.font_quill_8, &client.jag_title, x_STR_COMMA_LEN("q8.dat"));
    
    client_load_title_background();
    // NOTE: Moved out of `client_load_title_images`.
    // NOTE: Adding ".dat" suffix here instead of inside of `pix8_init`.
    pix8_init(&client.image_titlebox, &client.jag_title, x_STR_COMMA_LEN("titlebox.dat"), 0);
    pix8_init(&client.image_titlebutton, &client.jag_title, x_STR_COMMA_LEN("titlebutton.dat"), 0);
    for (int i = 0; i < 12; ++i) {
        pix8_init(&client.image_runes[i], &client.jag_title, x_STR_COMMA_LEN("runes.dat"), i);
    }
    client_load_title_images();

    // TODO
}

static int client_gameshell_init_application(int32_t height, int32_t width, int32_t *draw_area_data) {
    client.gameshell.screen_width = width;
    client.gameshell.screen_height = height;
    if (platform_frame_init(width, height) < 0) { // java: this.frame = new ViewBox(this.screenWidth, this.screenHeight, this);
        return -1;
    }
    pix_map_init(&client.gameshell.draw_area, width, height, draw_area_data);

    // java: this.startThread(this, 1);
    // This starts run() which calls `this.drawProgress(0, "Loading...");` which
    // in turn calls `this.loadTitle();` before attempting to draw progress
    // using system fonts. NOTE: We avoid system fonts completely.
    client_load_title();
    // run() continues by calling `this.load();`
    client_load0();
    return 0;
}

static void client_init(void) {
    client.flame_active = false;
    client.redraw_frame = false;
    // TODO: default initialisation of client fields
}

int client_main(int argc, char **argv) {
    // NOTE: Running all static initialisation blocks from here.
    pix_font_static_init();

    platform_print(x_STR_COMMA_LEN("RS2 user client - release #244\n"));

    if (argc == 6) {
        char *parse;
        int32_t parse_len;
        int64_t parsed;
        
        parse = argv[1];
        parse_len = util_str_to_int(parse, util_INT32_MAX_CHARS, &parsed);
        if (parse_len <= 0 || parse[parse_len] != '\0' || (uint64_t)parsed > INT32_MAX) return 1;
        client_node_id = (int32_t)parsed;

        parse = argv[2];
        parse_len = util_str_to_int(parse, util_INT32_MAX_CHARS, &parsed);
        if (parse_len <= 0 || parse[parse_len] != '\0' || (uint64_t)parsed > INT32_MAX) return 1;
        client_port_offset = (int32_t)parsed;

        if (util_cstr_cmp(argv[3], "lowmem") == 0) {
            client_set_low_memory();
        } else if (util_cstr_cmp(argv[3], "highmem") == 0) {
            client_set_high_memory();
        } else {
            platform_print(x_STR_COMMA_LEN("Usage: node-id, port-offset, [lowmem/highmem], [free/members], storeid\n"));
            return -1;
        }

        if (util_cstr_cmp(argv[4], "free") == 0) {
            client_members_world = false;
        } else if (util_cstr_cmp(argv[4], "members") == 0) {
            client_members_world = true;
        } else {
            platform_print(x_STR_COMMA_LEN("Usage: node-id, port-offset, [lowmem/highmem], [free/members], storeid\n"));
            return -1;
        }

        parse = argv[5];
        parse_len = util_str_to_int(parse, util_INT32_MAX_CHARS, &parsed);
        if (parse_len <= 0 || parse[parse_len] != '\0' || (uint64_t)parsed > INT32_MAX) return 1;
        // TODO:
        // SignLink.storeid = Integer.parseInt(args[4]);
        // SignLink.startpriv(InetAddress.getLocalHost());

        client_init();
        static int32_t draw_area_data[503 * 765]; // TODO: Move into client.s.game?
        return client_gameshell_init_application(503, 765, &draw_area_data[0]);
    } else {
        platform_print(x_STR_COMMA_LEN("Usage: node-id, port-offset, [lowmem/highmem], [free/members], storeid\n"));
        return -1;
    }
}
