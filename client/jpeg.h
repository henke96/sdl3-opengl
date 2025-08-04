#define jpeg_IMG_N_COMP 3

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

struct jpeg_huffman {
   uint8_t  fast[1 << FAST_BITS];
   uint16_t code[256];
   uint8_t  values[256];
   uint8_t  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
};

struct jpeg {
   uint32_t img_x, img_y;
   int img_n;

   uint8_t *img_buffer, *img_buffer_end;

   struct jpeg_huffman huff_dc[4];
   struct jpeg_huffman huff_ac[4];
   uint16_t dequant[4][64];
   int16_t fast_ac[4][1 << FAST_BITS];

   // sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

   // definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
      uint8_t *data;
      uint8_t *linebuf;
   } img_comp[4];

   uint32_t   code_buffer; // jpeg entropy-coded buffer
   int            code_bits;   // number of valid bits
   unsigned char  marker;      // marker seen while filling entropy buffer
   int            nomore;      // flag if we saw a marker so must stop

   int            eob_run;

   int scan_n, order[4];
};

int32_t *jpeg_decode(
   struct jpeg *jpeg,
   uint8_t *buffer,
   ptrdiff_t buffer_length,
   int32_t *out_width,
   int32_t *out_height
);
