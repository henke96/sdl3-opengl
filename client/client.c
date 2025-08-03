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

struct client_gameshell {
    struct pix_map draw_area;
    int32_t screen_width;
    int32_t screen_height;
};

struct client {
    struct client_gameshell gameshell;
    struct pix_map image_title0;
    struct pix_map image_title1;
    struct pix_map image_title2;
    struct pix_map image_title3;
    struct pix_map image_title4;
    struct pix_map image_title5;
    struct pix_map image_title6;
    struct pix_map image_title7;
    struct pix_map image_title8;
    struct pix_font font_plain_11;
    struct pix_font font_plain_12;
    struct pix_font font_bold_12;
    struct pix_font font_quill_8;
    struct jagfile jag_title;
    struct file_stream file_streams[5];
    int32_t jag_checksum[9];
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

    static int32_t image_title0_data[128 * 265];
    pix_map_init(&client.image_title0, 128, 265, &image_title0_data[0]);
    pix2d_clear();

    static int32_t image_title1_data[128 * 265];
    pix_map_init(&client.image_title1, 128, 265, &image_title1_data[0]);
    pix2d_clear();

    static int32_t image_title2_data[509 * 171];
    pix_map_init(&client.image_title2, 509, 171, &image_title2_data[0]);
    pix2d_clear();

    static int32_t image_title3_data[360 * 132];
    pix_map_init(&client.image_title3, 360, 132, &image_title3_data[0]);
    pix2d_clear();

    static int32_t image_title4_data[360 * 200];
    pix_map_init(&client.image_title4, 360, 200, &image_title4_data[0]);
    pix2d_clear();

    static int32_t image_title5_data[202 * 238];
    pix_map_init(&client.image_title5, 202, 238, &image_title5_data[0]);
    pix2d_clear();

    static int32_t image_title6_data[203 * 238];
    pix_map_init(&client.image_title6, 203, 238, &image_title6_data[0]);
    pix2d_clear();

    static int32_t image_title7_data[74 * 94];
    pix_map_init(&client.image_title7, 74, 94, &image_title7_data[0]);
    pix2d_clear();

    static int32_t image_title8_data[75 * 94];
    pix_map_init(&client.image_title8, 75, 94, &image_title8_data[0]);
    pix2d_clear();

    // TODO:
    // if (this.jagTitle != null) {
    //     this.loadTitleBackground();
    //     this.loadTitleImages();
    // }

    client.redraw_frame = true;
}

static void client_draw_progress(int32_t percent, char *message) {
    // TODO:
    // this.lastProgressPercent = percent;
    // this.lastProgressMessage = message;

    // NOTE: client_load_title() moved out of this function.

    // TODO
}

// java: java.util.zip.CRC32
static int32_t client_crc32(uint8_t *data, ptrdiff_t data_length) {
    uint32_t crc = 0xFFFFFFFF;
    for (ptrdiff_t i = 0; i < data_length; ++i) {
        crc = crc ^ (uint32_t)data[i];
        for (int j = 0; j < 8; ++j) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return (int32_t)~crc;
}

static void client_get_jag_file(int32_t crc, char *name, int32_t file, char *display_name, struct jagfile *out_file) {
    uint8_t *data = NULL;
    int32_t retry = 5;

    // TODO: if (this.fileStreams[0] != null) {
    ptrdiff_t data_length;
    data = file_stream_read(&client.file_streams[0], file, &data_length);

    if (data != NULL) {
        int32_t checksum = client_crc32(data, data_length);
        if (crc != checksum) data = NULL;
    }

    if (data != NULL) {
        jagfile_init(out_file, data, data_length);
        return;
    }

    // TODO: Download loop..
    for (;;);
}

// java: load() up until TODO
static void client_load1(void) {
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
    client_load1();
    return 0;
}

static void client_init(void) {
    // TODO: default initialisation of client fields
}

int client_main(int argc, char **argv) {
    // NOTE: Running all static initialisation blocks from here.
    pix_font_static_init();

    platform_print(x_STR_COMMA_LEN("RS2 user client - release #244\n"));

    if (argc == 6) {
        char *parse;
        ptrdiff_t parse_len;
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
        static int32_t draw_area_data[503 * 765];
        return client_gameshell_init_application(503, 765, &draw_area_data[0]);
    } else {
        platform_print(x_STR_COMMA_LEN("Usage: node-id, port-offset, [lowmem/highmem], [free/members], storeid\n"));
        return -1;
    }
}
