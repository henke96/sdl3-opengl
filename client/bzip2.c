#include "platform.h"
#include "x.h"
#include "bzip2.h"

#define bzip2_MTFA_SIZE 4096
#define bzip2_MTFL_SIZE 16
#define bzip2_BZ_MAX_ALPHA_SIZE 258
#define bzip2_BZ_MAX_CODE_LEN 23
#define bzip2_BZ_N_GROUPS 6
#define bzip2_BZ_G_SIZE 50
#define bzip2_BZ_MAX_SELECTORS (2 + (900000 / bzip2_BZ_G_SIZE)) // 18002
#define bzip2_BZ_RUNA 0
#define bzip2_BZ_RUNB 1

#define bzip2_BLOCK_SIZE_100K 1 // NOTE: Moved out from bzip2_state since it was always set to 1.
static int32_t bzip2_state_tt[bzip2_BLOCK_SIZE_100K * 100000];

// NOTE: Removed the unused total_in and total_out counters.
static struct {
    uint8_t *stream;
    int32_t next_in;
    int32_t avail_in;
    uint8_t *decompressed;
    int32_t next_out;
    int32_t avail_out;
    uint8_t state_out_ch;
    int32_t state_out_len;
    bool block_randomised;
    uint32_t bs_buff;
    int32_t bs_live;
    int32_t curr_block_no;
    int32_t unzftab[256];
    int32_t cftab[257];
    int32_t cftab_copy[257];
    bool in_use[256];
    bool in_use16[16];
    uint8_t seq_to_unseq[256];
    uint8_t mtfa[bzip2_MTFA_SIZE];
    int32_t mtfbase[256 / bzip2_MTFL_SIZE];
    int8_t selector[bzip2_BZ_MAX_SELECTORS];
    uint8_t selector_mtf[bzip2_BZ_MAX_SELECTORS];
    int8_t len[bzip2_BZ_N_GROUPS][bzip2_BZ_MAX_ALPHA_SIZE];
    int32_t limit[bzip2_BZ_N_GROUPS][bzip2_BZ_MAX_ALPHA_SIZE];
    int32_t base[bzip2_BZ_N_GROUPS][bzip2_BZ_MAX_ALPHA_SIZE];
    int32_t perm[bzip2_BZ_N_GROUPS][bzip2_BZ_MAX_ALPHA_SIZE];
    int32_t min_lens[bzip2_BZ_N_GROUPS];
    uint32_t orig_ptr;
    int32_t t_pos;
    int32_t k0;
    int32_t c_nblock_used;
    int32_t n_in_use;
    int32_t save_nblock;
} bzip2_state;

static int32_t bzip2_get_bits(int32_t n) {
    while (bzip2_state.bs_live < n) {
        bzip2_state.bs_buff = bzip2_state.bs_buff << 8 | bzip2_state.stream[bzip2_state.next_in];
        bzip2_state.bs_live += 8;

        ++bzip2_state.next_in;
        --bzip2_state.avail_in;
    }

    int32_t value = bzip2_state.bs_buff >> (bzip2_state.bs_live - n) & ((0x1 << n) - 1);
    bzip2_state.bs_live -= n;
    return value;
}

static uint8_t bzip2_get_unsigned_byte(void) {
    return (uint8_t)bzip2_get_bits(8);
}

static uint8_t bzip2_get_bit(void) {
    return (uint8_t)bzip2_get_bits(1);
}

static void bzip2_make_maps(void) {
    bzip2_state.n_in_use = 0;
    for (int i = 0; i < 256; ++i) {
        if (bzip2_state.in_use[i]) {
            bzip2_state.seq_to_unseq[bzip2_state.n_in_use] = (uint8_t)i;
            ++bzip2_state.n_in_use;
        }
    }
}

static void bzip2_create_decode_tables(int32_t *limit, int32_t *base, int32_t *perm, int8_t *length, int32_t min_len, int32_t max_len, int32_t alpha_size) {
    int32_t pp = 0;

    for (int32_t i = min_len; i <= max_len; ++i) {
        for (int32_t j = 0; j < alpha_size; ++j) {
            if (length[j] == i) {
                perm[pp] = j;
                ++pp;
            }
        }
    }

    for (int i = 0; i < bzip2_BZ_MAX_CODE_LEN; ++i) {
        base[i] = 0;
    }

    for (int32_t i = 0; i < alpha_size; ++i) {
        ++base[length[i] + 1];
    }

    for (int i = 1; i < bzip2_BZ_MAX_CODE_LEN; ++i) {
        base[i] += base[i - 1];
    }

    for (int i = 0; i < bzip2_BZ_MAX_CODE_LEN; ++i) {
        limit[i] = 0;
    }

    int32_t vec = 0;
    for (int32_t i = min_len; i <= max_len; ++i) {
        vec += base[i + 1] - base[i];
        limit[i] = vec - 1;
        vec <<= 1;
    }

    for (int i = min_len + 1; i <= max_len; ++i) {
        base[i] = ((limit[i - 1] + 1) << 1) - base[i];
    }
}

static void bzip2_finish(void) {
    uint8_t c_state_out_ch = bzip2_state.state_out_ch;
    int32_t c_state_out_len = bzip2_state.state_out_len;
    int32_t c_nblock_used = bzip2_state.c_nblock_used;
    int32_t c_k0 = bzip2_state.k0;
    int c_t_pos = bzip2_state.t_pos;
    uint8_t *cs_decompressed = &bzip2_state.decompressed[0];
    int32_t cs_next_out = bzip2_state.next_out;
    int32_t cs_avail_out = bzip2_state.avail_out;
    int32_t s_save_nblock_pp = bzip2_state.save_nblock + 1;

    while (true) {
        if (c_state_out_len > 0) {
            while (true) {
                if (cs_avail_out == 0) goto done;

                if (c_state_out_len == 1) {
                    if (cs_avail_out == 0) {
                        c_state_out_len = 1;
                        goto done;
                    }

                    cs_decompressed[cs_next_out] = c_state_out_ch;
                    ++cs_next_out;
                    --cs_avail_out;
                    break;
                }

                cs_decompressed[cs_next_out] = c_state_out_ch;
                --c_state_out_len;
                ++cs_next_out;
                --cs_avail_out;
            }
        }

        bool next = true;
        uint8_t k1;
        while (next) {
            next = false;
            if (c_nblock_used == s_save_nblock_pp) {
                c_state_out_len = 0;
                goto done;
            }

            c_state_out_ch = (uint8_t)c_k0;
            c_t_pos = bzip2_state_tt[c_t_pos];
            k1 = (uint8_t)(c_t_pos & 0xFF);
            c_t_pos >>= 8;
            ++c_nblock_used;

            if (c_k0 != k1) {
                c_k0 = k1;
                if (cs_avail_out == 0) {
                    c_state_out_len = 1;
                    goto done;
                }

                cs_decompressed[cs_next_out] = c_state_out_ch;
                ++cs_next_out;
                --cs_avail_out;
                next = true;
            } else if (c_nblock_used == s_save_nblock_pp) {
                if (cs_avail_out == 0) {
                    c_state_out_len = 1;
                    goto done;
                }

                cs_decompressed[cs_next_out] = c_state_out_ch;
                ++cs_next_out;
                --cs_avail_out;
                next = true;
            }
        }

        c_state_out_len = 2;
        c_t_pos = bzip2_state_tt[c_t_pos];
        k1 = (uint8_t)(c_t_pos & 0xFF);
        c_t_pos >>= 8;
        ++c_nblock_used;

        if (c_nblock_used != s_save_nblock_pp) {
            if (c_k0 == k1) {
                c_state_out_len = 3;
                c_t_pos = bzip2_state_tt[c_t_pos];
                k1 = (uint8_t)(c_t_pos & 0xFF);
                c_t_pos >>= 8;
                ++c_nblock_used;

                if (c_nblock_used != s_save_nblock_pp) {
                    if (c_k0 == k1) {
                        c_t_pos = bzip2_state_tt[c_t_pos];
                        k1 = (uint8_t)(c_t_pos & 0xFF);
                        c_t_pos >>= 8;
                        ++c_nblock_used;

                        c_state_out_len = (k1 & 0xFF) + 4;
                        c_t_pos = bzip2_state_tt[c_t_pos];
                        c_k0 = (uint8_t)(c_t_pos & 0xFF);
                        c_t_pos >>= 8;
                        ++c_nblock_used;
                    } else {
                        c_k0 = k1;
                    }
                }
            } else {
                c_k0 = k1;
            }
        }
    }
    done:;

    bzip2_state.state_out_ch = c_state_out_ch;
    bzip2_state.state_out_len = c_state_out_len;
    bzip2_state.c_nblock_used = c_nblock_used;
    bzip2_state.k0 = c_k0;
    bzip2_state.t_pos = c_t_pos;
    bzip2_state.decompressed = cs_decompressed;
    bzip2_state.next_out = cs_next_out;
    bzip2_state.avail_out = cs_avail_out;
}

static void bzip2_do_decompress(void) {
    int32_t g_minlen = 0;
    int32_t *g_limit = NULL;
    int32_t *g_base = NULL;
    int32_t *g_perm = NULL;

    // NOTE: Avoiding dynamically allocating bzip2_state_tt.

    bool reading = true;
    while (reading) {
        uint8_t uc = bzip2_get_unsigned_byte();
        if (uc == 0x17) {
            return;
        }

        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();

        ++bzip2_state.curr_block_no;

        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();
        uc = bzip2_get_unsigned_byte();

        uc = bzip2_get_bit();
        if (uc == 0) {
            bzip2_state.block_randomised = false;
        } else {
            bzip2_state.block_randomised = true;
        }

        if (bzip2_state.block_randomised) {
            platform_print(x_STR_COMMA_LEN("PANIC! RANDOMISED BLOCK!"));
        }

        bzip2_state.orig_ptr = 0;
        uc = bzip2_get_unsigned_byte();
        bzip2_state.orig_ptr = bzip2_state.orig_ptr << 8 | uc;
        uc = bzip2_get_unsigned_byte();
        bzip2_state.orig_ptr = bzip2_state.orig_ptr << 8 | uc;
        uc = bzip2_get_unsigned_byte();
        bzip2_state.orig_ptr = bzip2_state.orig_ptr << 8 | uc;

        // Receive the mapping table.
        for (int i = 0; i < 16; ++i) {
            uc = bzip2_get_bit();
            if (uc == 1) {
                bzip2_state.in_use16[i] = true;
            } else {
                bzip2_state.in_use16[i] = false;
            }
        }

        for (int i = 0; i < 256; ++i) {
            bzip2_state.in_use[i] = false;
        }

        for (int i = 0; i < 16; ++i) {
            if (bzip2_state.in_use16[i]) {
                for (int j = 0; j < 16; ++j) {
                    uc = bzip2_get_bit();
                    if (uc == 1) {
                        bzip2_state.in_use[i * 16 + j] = true;
                    }
                }
            }
        }
        bzip2_make_maps();
        int32_t alpha_size = bzip2_state.n_in_use + 2;

        int32_t n_groups = bzip2_get_bits(3);
        int32_t n_selectors = bzip2_get_bits(15);
        for (int32_t i = 0; i < n_selectors; ++i) {
            int32_t j = 0;
            while (true) {
                uc = bzip2_get_bit();
                if (uc == 0) {
                    bzip2_state.selector_mtf[i] = (int8_t)j;
                    break;
                }

                ++j;
            }
        }

        // Undo the MTF values for the selectors.
        uint8_t pos[bzip2_BZ_N_GROUPS];
        int8_t v = 0;
        while (v < n_groups) {
            pos[v] = v;
            ++v;
        }

        for (int32_t i = 0; i < n_selectors; ++i) {
            v = bzip2_state.selector_mtf[i];
            uint8_t tmp = pos[v];
            while (v > 0) {
                pos[v] = pos[v - 1];
                --v;
            }
            pos[0] = tmp;
            bzip2_state.selector[i] = tmp;
        }

        // Now the coding tables
        for (int32_t t = 0; t < n_groups; t++) {
            int32_t curr = bzip2_get_bits(5);

            for (int32_t i = 0; i < alpha_size; ++i) {
                while (true) {
                    uc = bzip2_get_bit();
                    if (uc == 0) {
                        bzip2_state.len[t][i] = (int8_t)curr;
                        break;
                    }

                    uc = bzip2_get_bit();
                    if (uc == 0) {
                        ++curr;
                    } else {
                        --curr;
                    }
                }
            }
        }

        // Create the Huffman decoding tables
        for (int32_t t = 0; t < n_groups; t++) {
            int8_t min_len = 32;
            int8_t max_len = 0;

            for (int32_t i = 0; i < alpha_size; ++i) {
                if (bzip2_state.len[t][i] > max_len) {
                    max_len = bzip2_state.len[t][i];
                }

                if (bzip2_state.len[t][i] < min_len) {
                    min_len = bzip2_state.len[t][i];
                }
            }

            bzip2_create_decode_tables(
                &bzip2_state.limit[t][0],
                &bzip2_state.base[t][0],
                &bzip2_state.perm[t][0],
                &bzip2_state.len[t][0],
                min_len,
                max_len,
                alpha_size
            );
            bzip2_state.min_lens[t] = min_len;
        }

        // Now the MTF values.
        int32_t eob = bzip2_state.n_in_use + 1;
        int32_t group_no = -1;
        int8_t group_pos = 0;

        for (int i = 0; i <= 255; ++i) {
            bzip2_state.unzftab[i] = 0;
        }

        int32_t kk = bzip2_MTFA_SIZE - 1;
        for (int32_t ii = 256 / bzip2_MTFL_SIZE - 1; ii >= 0; ii--) {
            for (int32_t jj = bzip2_MTFL_SIZE - 1; jj >= 0; jj--) {
                bzip2_state.mtfa[kk] = (uint8_t)(ii * 16 + jj);
                --kk;
            }

            bzip2_state.mtfbase[ii] = kk + 1;
        }

        int32_t nblock = 0;

        if (group_pos == 0) {
            ++group_no;
            group_pos = 50;
            int8_t g_sel = bzip2_state.selector[group_no];
            g_minlen = bzip2_state.min_lens[g_sel];
            g_limit = bzip2_state.limit[g_sel];
            g_perm = bzip2_state.perm[g_sel];
            g_base = bzip2_state.base[g_sel];
        }

        int32_t g_pos = group_pos - 1;
        int32_t zn = g_minlen;
        int32_t zvec;
        uint8_t zj;
        for (zvec = bzip2_get_bits(g_minlen); zvec > g_limit[zn]; zvec = zvec << 1 | zj) {
            ++zn;
            zj = bzip2_get_bit();
        }

        int32_t next_sym = g_perm[zvec - g_base[zn]];
        while (eob != next_sym) {
            if (next_sym == bzip2_BZ_RUNA || next_sym == bzip2_BZ_RUNB) {
                int32_t es = -1;
                int32_t n = 1;

                do {
                    if (next_sym == bzip2_BZ_RUNA) {
                        es += n;
                    } else if (next_sym == bzip2_BZ_RUNB) {
                        es += n * 2;
                    }

                    n *= 2;

                    if (g_pos == 0) {
                        ++group_no;
                        g_pos = 50;
                        int8_t g_sel = bzip2_state.selector[group_no];
                        g_minlen = bzip2_state.min_lens[g_sel];
                        g_limit = bzip2_state.limit[g_sel];
                        g_perm = bzip2_state.perm[g_sel];
                        g_base = bzip2_state.base[g_sel];
                    }

                    --g_pos;
                    zn = g_minlen;
                    for (zvec = bzip2_get_bits(g_minlen); zvec > g_limit[zn]; zvec = zvec << 1 | zj) {
                        ++zn;
                        zj = bzip2_get_bit();
                    }

                    next_sym = g_perm[zvec - g_base[zn]];
                } while (next_sym == bzip2_BZ_RUNA || next_sym == bzip2_BZ_RUNB);

                ++es;
                uint8_t var84 = bzip2_state.seq_to_unseq[bzip2_state.mtfa[bzip2_state.mtfbase[0]] & 0xFF];
                bzip2_state.unzftab[var84] += es;

                while (es > 0) {
                    bzip2_state_tt[nblock] = var84;
                    ++nblock;
                    --es;
                }
            } else {
                int32_t nn = next_sym - 1;

                if (nn < bzip2_MTFL_SIZE) {
                    // Avoid general-case expense.
                    int32_t pp = bzip2_state.mtfbase[0];
                    uc = bzip2_state.mtfa[nn + pp];

                    while (nn > 3) {
                        int32_t z = nn + pp;
                        bzip2_state.mtfa[z] = bzip2_state.mtfa[z - 1];
                        bzip2_state.mtfa[z - 1] = bzip2_state.mtfa[z - 2];
                        bzip2_state.mtfa[z - 2] = bzip2_state.mtfa[z - 3];
                        bzip2_state.mtfa[z - 3] = bzip2_state.mtfa[z - 4];
                        nn -= 4;
                    }

                    while (nn > 0) {
                        bzip2_state.mtfa[nn + pp] = bzip2_state.mtfa[nn + pp - 1];
                        --nn;
                    }

                    bzip2_state.mtfa[pp] = uc;
                } else {
                    // general case
                    int32_t lno = nn / bzip2_MTFL_SIZE;
                    int32_t off = nn % bzip2_MTFL_SIZE;

                    int32_t pp = bzip2_state.mtfbase[lno] + off;
                    uc = bzip2_state.mtfa[pp];

                    while (pp > bzip2_state.mtfbase[lno]) {
                        bzip2_state.mtfa[pp] = bzip2_state.mtfa[pp - 1];
                        --pp;
                    }

                    ++bzip2_state.mtfbase[lno];

                    while (lno > 0) {
                        --bzip2_state.mtfbase[lno];
                        bzip2_state.mtfa[bzip2_state.mtfbase[lno]] = bzip2_state.mtfa[bzip2_state.mtfbase[lno - 1] + 16 - 1];
                        --lno;
                    }

                    --bzip2_state.mtfbase[0];
                    bzip2_state.mtfa[bzip2_state.mtfbase[0]] = uc;

                    if (bzip2_state.mtfbase[0] == 0) {
                        kk = bzip2_MTFA_SIZE - 1;

                        for (int ii = 256 / bzip2_MTFL_SIZE - 1; ii >= 0; ii--) {
                            for (int32_t jj = bzip2_MTFL_SIZE - 1; jj >= 0; jj--) {
                                bzip2_state.mtfa[kk] = bzip2_state.mtfa[bzip2_state.mtfbase[ii] + jj];
                                --kk;
                            }

                            bzip2_state.mtfbase[ii] = kk + 1;
                        }
                    }
                }

                ++bzip2_state.unzftab[bzip2_state.seq_to_unseq[uc]];
                bzip2_state_tt[nblock] = bzip2_state.seq_to_unseq[uc];
                ++nblock;

                if (g_pos == 0) {
                    ++group_no;
                    g_pos = 50;
                    int8_t g_sel = bzip2_state.selector[group_no];
                    g_minlen = bzip2_state.min_lens[g_sel];
                    g_limit = bzip2_state.limit[g_sel];
                    g_perm = bzip2_state.perm[g_sel];
                    g_base = bzip2_state.base[g_sel];
                }

                --g_pos;
                zn = g_minlen;
                for (zvec = bzip2_get_bits(g_minlen); zvec > g_limit[zn]; zvec = zvec << 1 | zj) {
                    ++zn;
                    zj = bzip2_get_bit();
                }
                next_sym = g_perm[zvec - g_base[zn]];
            }
        }

        // Set up cftab to facilitate generation of T^(-1)

        // Actually generate cftab
        bzip2_state.cftab[0] = 0;

        for (int i = 1; i <= 256; ++i) {
            bzip2_state.cftab[i] = bzip2_state.unzftab[i - 1];
        }

        for (int i = 1; i <= 256; ++i) {
            bzip2_state.cftab[i] += bzip2_state.cftab[i - 1];
        }

        bzip2_state.state_out_len = 0;
        bzip2_state.state_out_ch = 0;

        for (int32_t i = 0; i < nblock; ++i) {
            uc = (uint8_t)(bzip2_state_tt[i] & 0xFF);
            bzip2_state_tt[bzip2_state.cftab[uc]] |= i << 8;
            ++bzip2_state.cftab[uc];
        }

        bzip2_state.t_pos = bzip2_state_tt[bzip2_state.orig_ptr] >> 8;
        bzip2_state.c_nblock_used = 0;

        bzip2_state.t_pos = bzip2_state_tt[bzip2_state.t_pos];
        bzip2_state.k0 = bzip2_state.t_pos & 0xFF;
        bzip2_state.t_pos >>= 8;
        ++bzip2_state.c_nblock_used;

        bzip2_state.save_nblock = nblock;
        bzip2_finish();

        if (bzip2_state.save_nblock + 1 == bzip2_state.c_nblock_used && bzip2_state.state_out_len == 0) {
            reading = true;
        } else {
            reading = false;
        }
    }
}

int32_t bzip2_decompress(uint8_t *decompressed, int32_t length, uint8_t *stream, int32_t avail_in, int32_t next_in) {
    bzip2_state.stream = stream;
    bzip2_state.next_in = next_in;
    bzip2_state.decompressed = decompressed;
    bzip2_state.next_out = 0;
    bzip2_state.avail_in = avail_in;
    bzip2_state.avail_out = length;
    bzip2_state.bs_live = 0;
    bzip2_state.bs_buff = 0;
    bzip2_state.curr_block_no = 0;
    bzip2_do_decompress();
    return length - bzip2_state.avail_out;
}

