// Based on stb_image - v2.30 - public domain image loader - http://nothings.org/stb
#include "platform.h"
#include "jpeg.h"

#define stbi_jbias(n) (((uint32_t)0xFFFFFFFF << n) + 1)
#define stbi_bmask(n) (((uint32_t)1 << n) - 1)
#define stbi_lrot(x, y)  (((x) << (y)) | ((x) >> (-(y) & 31)))

#ifndef STBI_MAX_DIMENSIONS
#define STBI_MAX_DIMENSIONS (1 << 24)
#endif

// Returns 1 if "a*b" has no negative factors and doesn't overflow.
static int stbi__mul2sizes_valid(int a, int b)
{
   if (a < 0 || b < 0) return 0;
   if (b == 0) return 1; // mul-by-0 is always safe
   return a <= INT_MAX / b;
}

// Returns 1 if "a*b*c" has no negative factors and doesn't overflow.
static int stbi__mul3sizes_valid(int a, int b, int c)
{
   return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a * b, c);
}

// returns 1 if the sum of two signed ints is valid (between -2^31 and 2^31-1 inclusive), 0 on overflow.
static int stbi__addints_valid(int a, int b)
{
   if ((a >= 0) != (b >= 0)) return 1; // a and b have different signs, so no overflow
   if (a < 0 && b < 0) return a >= INT_MIN - b; // same as a + b >= INT_MIN; INT_MIN - b cannot overflow since b < 0.
   return a <= INT_MAX - b;
}

// returns 1 if the product of two ints fits in a signed short, 0 on overflow.
static int stbi__mul2shorts_valid(int a, int b)
{
   if (b == 0 || b == -1) return 1; // multiplication by 0 is always 0; check for -1 so SHRT_MIN/b doesn't overflow
   if ((a >= 0) == (b >= 0)) return a <= SHRT_MAX/b; // product is positive, so similar to mul2sizes_valid
   if (b < 0) return a <= SHRT_MIN / b; // same as a * b >= SHRT_MIN
   return a >= SHRT_MIN / b;
}

// stbi__err - error
// stbi__errpuc - error returning pointer to unsigned char

#define stbi__err(x,y)  0

static uint8_t stbi__get8(struct jpeg *s)
{
   if (s->img_buffer < s->img_buffer_end)
      return *s->img_buffer++;
   return 0;
}

static void stbi__skip(struct jpeg *s, int n)
{
   if (n == 0) return;  // already there!
   if (n < 0) {
      s->img_buffer = s->img_buffer_end;
      return;
   }
   s->img_buffer += n;
}


static int stbi__get16be(struct jpeg *s)
{
   int z = stbi__get8(s);
   return (z << 8) + stbi__get8(s);
}

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder
//
//    simple implementation
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - uses a lot of intermediate memory, could cache poorly

static int stbi__build_huffman(struct jpeg_huffman *h, int *count)
{
   int i,j,k=0;
   unsigned int code;
   // build size list for each symbol (from JPEG spec)
   for (i=0; i < 16; ++i) {
      for (j=0; j < count[i]; ++j) {
         h->size[k++] = (uint8_t) (i+1);
         if(k >= 257) return stbi__err("bad size list","Corrupt JPEG");
      }
   }
   h->size[k] = 0;

   // compute actual symbols (from jpeg spec)
   code = 0;
   k = 0;
   for(j=1; j <= 16; ++j) {
      // compute delta to add to code to compute symbol id
      h->delta[j] = k - code;
      if (h->size[k] == j) {
         while (h->size[k] == j)
            h->code[k++] = (uint16_t) (code++);
         if (code-1 >= (1u << j)) return stbi__err("bad code lengths","Corrupt JPEG");
      }
      // compute largest code + 1 for this size, preshifted as needed later
      h->maxcode[j] = code << (16-j);
      code <<= 1;
   }
   h->maxcode[j] = 0xffffffff;

   // build non-spec acceleration table; 255 is flag for not-accelerated
   memset(h->fast, 255, 1 << FAST_BITS);
   for (i=0; i < k; ++i) {
      int s = h->size[i];
      if (s <= FAST_BITS) {
         int c = h->code[i] << (FAST_BITS-s);
         int m = 1 << (FAST_BITS-s);
         for (j=0; j < m; ++j) {
            h->fast[c+j] = (uint8_t) i;
         }
      }
   }
   return 1;
}

// build a table that decodes both magnitude and value of small ACs in
// one go.
static void stbi__build_fast_ac(int16_t *fast_ac, struct jpeg_huffman *h)
{
   int i;
   for (i=0; i < (1 << FAST_BITS); ++i) {
      uint8_t fast = h->fast[i];
      fast_ac[i] = 0;
      if (fast < 255) {
         int rs = h->values[fast];
         int run = (rs >> 4) & 15;
         int magbits = rs & 15;
         int len = h->size[fast];

         if (magbits && len + magbits <= FAST_BITS) {
            // magnitude code followed by receive_extend code
            int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
            int m = 1 << (magbits - 1);
            if (k < m) k += (~0U << magbits) + 1;
            // if the result is small enough, we can fit it in fast_ac table
            if (k >= -128 && k <= 127)
               fast_ac[i] = (int16_t) ((k * 256) + (run * 16) + (len + magbits));
         }
      }
   }
}

static void stbi__grow_buffer_unsafe(struct jpeg *j)
{
   do {
      unsigned int b = j->nomore ? 0 : stbi__get8(j);
      if (b == 0xff) {
         int c = stbi__get8(j);
         while (c == 0xff) c = stbi__get8(j); // consume fill bytes
         if (c != 0) {
            j->marker = (unsigned char) c;
            j->nomore = 1;
            return;
         }
      }
      j->code_buffer |= b << (24 - j->code_bits);
      j->code_bits += 8;
   } while (j->code_bits <= 24);
}

// decode a jpeg huffman value from the bitstream
static int stbi__jpeg_huff_decode(struct jpeg *j, struct jpeg_huffman *h)
{
   unsigned int temp;
   int c,k;

   if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      int s = h->size[k];
      if (s > j->code_bits)
         return -1;
      j->code_buffer <<= s;
      j->code_bits -= s;
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   temp = j->code_buffer >> 16;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      j->code_bits -= 16;
      return -1;
   }

   if (k > j->code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((j->code_buffer >> (32 - k)) & stbi_bmask(k)) + h->delta[k];
   if(c < 0 || c >= 256) // symbol id out of bounds!
       return -1;

   // convert the id to a symbol
   j->code_bits -= k;
   j->code_buffer <<= k;
   return h->values[c];
}

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
static int stbi__extend_receive(struct jpeg *j, int n)
{
   unsigned int k;
   int sgn;
   if (j->code_bits < n) stbi__grow_buffer_unsafe(j);
   if (j->code_bits < n) return 0; // ran out of bits from stream, return 0s intead of continuing

   sgn = j->code_buffer >> 31; // sign bit always in MSB; 0 if MSB clear (positive), 1 if MSB set (negative)
   k = stbi_lrot(j->code_buffer, n);
   j->code_buffer = k & ~stbi_bmask(n);
   k &= stbi_bmask(n);
   j->code_bits -= n;
   return k + (stbi_jbias(n) & (sgn - 1));
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static const uint8_t stbi__jpeg_dezigzag[64+15] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
   // let corrupt input sample past end
   63, 63, 63, 63, 63, 63, 63, 63,
   63, 63, 63, 63, 63, 63, 63
};

// decode one 64-entry block--
static int stbi__jpeg_decode_block(struct jpeg *j, short data[64], struct jpeg_huffman *hdc, struct jpeg_huffman *hac, int16_t *fac, int b, uint16_t *dequant)
{
   int diff,dc,k;
   int t;

   if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
   t = stbi__jpeg_huff_decode(j, hdc);
   if (t < 0 || t > 15) return stbi__err("bad huffman code","Corrupt JPEG");

   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? stbi__extend_receive(j, t) : 0;
   if (!stbi__addints_valid(j->img_comp[b].dc_pred, diff)) return stbi__err("bad delta","Corrupt JPEG");
   dc = j->img_comp[b].dc_pred + diff;
   j->img_comp[b].dc_pred = dc;
   if (!stbi__mul2shorts_valid(dc, dequant[0])) return stbi__err("can't merge dc and ac", "Corrupt JPEG");
   data[0] = (short) (dc * dequant[0]);

   // decode AC components, see JPEG spec
   k = 1;
   do {
      unsigned int zig;
      int c,r,s;
      if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
      c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
      r = fac[c];
      if (r) { // fast-AC path
         k += (r >> 4) & 15; // run
         s = r & 15; // combined length
         if (s > j->code_bits) return stbi__err("bad huffman code", "Combined length longer than code bits available");
         j->code_buffer <<= s;
         j->code_bits -= s;
         // decode into unzigzag'd location
         zig = stbi__jpeg_dezigzag[k++];
         data[zig] = (short) ((r >> 8) * dequant[zig]);
      } else {
         int rs = stbi__jpeg_huff_decode(j, hac);
         if (rs < 0) return stbi__err("bad huffman code","Corrupt JPEG");
         s = rs & 15;
         r = rs >> 4;
         if (s == 0) {
            if (rs != 0xf0) break; // end block
            k += 16;
         } else {
            k += r;
            // decode into unzigzag'd location
            zig = stbi__jpeg_dezigzag[k++];
            data[zig] = (short) (stbi__extend_receive(j,s) * dequant[zig]);
         }
      }
   } while (k < 64);
   return 1;
}

// take a -128..127 value and stbi__clamp it and convert to 0..255
static uint8_t stbi__clamp(int x)
{
   // trick to use a single test to catch both cases
   if ((unsigned int) x > 255) {
      if (x < 0) return 0;
      if (x > 255) return 255;
   }
   return (uint8_t) x;
}

#define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
#define stbi__fsh(x)  ((x) * 4096)

// derived from jidctint -- DCT_ISLOW
#define STBI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * stbi__f2f(0.5411961f);       \
   t2 = p1 + p3*stbi__f2f(-1.847759065f);      \
   t3 = p1 + p2*stbi__f2f( 0.765366865f);      \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = stbi__fsh(p2+p3);                      \
   t1 = stbi__fsh(p2-p3);                      \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*stbi__f2f( 1.175875602f);      \
   t0 = t0*stbi__f2f( 0.298631336f);           \
   t1 = t1*stbi__f2f( 2.053119869f);           \
   t2 = t2*stbi__f2f( 3.072711026f);           \
   t3 = t3*stbi__f2f( 1.501321110f);           \
   p1 = p5 + p1*stbi__f2f(-0.899976223f);      \
   p2 = p5 + p2*stbi__f2f(-2.562915447f);      \
   p3 = p3*stbi__f2f(-1.961570560f);           \
   p4 = p4*stbi__f2f(-0.390180644f);           \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;

static void stbi__idct_block(uint8_t *out, int out_stride, short data[64])
{
   int i,val[64],*v=val;
   uint8_t *o;
   short *d = data;

   // columns
   for (i=0; i < 8; ++i,++d, ++v) {
      // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
      if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0
           && d[40]==0 && d[48]==0 && d[56]==0) {
         //    no shortcut                 0     seconds
         //    (1|2|3|4|5|6|7)==0          0     seconds
         //    all separate               -0.047 seconds
         //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
         int dcterm = d[0]*4;
         v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
      } else {
         STBI__IDCT_1D(d[ 0],d[ 8],d[16],d[24],d[32],d[40],d[48],d[56])
         // constants scaled things up by 1<<12; let's bring them back
         // down, but keep 2 extra bits of precision
         x0 += 512; x1 += 512; x2 += 512; x3 += 512;
         v[ 0] = (x0+t3) >> 10;
         v[56] = (x0-t3) >> 10;
         v[ 8] = (x1+t2) >> 10;
         v[48] = (x1-t2) >> 10;
         v[16] = (x2+t1) >> 10;
         v[40] = (x2-t1) >> 10;
         v[24] = (x3+t0) >> 10;
         v[32] = (x3-t0) >> 10;
      }
   }

   for (i=0, v=val, o=out; i < 8; ++i,v+=8,o+=out_stride) {
      // no fast case since the first 1D IDCT spread components out
      STBI__IDCT_1D(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7])
      // constants scaled things up by 1<<12, plus we had 1<<2 from first
      // loop, plus horizontal and vertical each scale by sqrt(8) so together
      // we've got an extra 1<<3, so 1<<17 total we need to remove.
      // so we want to round that, which means adding 0.5 * 1<<17,
      // aka 65536. Also, we'll end up with -128 to 127 that we want
      // to encode as 0..255 by adding 128, so we'll add that before the shift
      x0 += 65536 + (128<<17);
      x1 += 65536 + (128<<17);
      x2 += 65536 + (128<<17);
      x3 += 65536 + (128<<17);
      // tried computing the shifts into temps, or'ing the temps to see
      // if any were out of range, but that was slower
      o[0] = stbi__clamp((x0+t3) >> 17);
      o[7] = stbi__clamp((x0-t3) >> 17);
      o[1] = stbi__clamp((x1+t2) >> 17);
      o[6] = stbi__clamp((x1-t2) >> 17);
      o[2] = stbi__clamp((x2+t1) >> 17);
      o[5] = stbi__clamp((x2-t1) >> 17);
      o[3] = stbi__clamp((x3+t0) >> 17);
      o[4] = stbi__clamp((x3-t0) >> 17);
   }
}

#define STBI__MARKER_none  0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static uint8_t stbi__get_marker(struct jpeg *j)
{
   uint8_t x;
   if (j->marker != STBI__MARKER_none) { x = j->marker; j->marker = STBI__MARKER_none; return x; }
   x = stbi__get8(j);
   if (x != 0xff) return STBI__MARKER_none;
   while (x == 0xff)
      x = stbi__get8(j); // consume repeated 0xff fill bytes
   return x;
}

// after a restart interval, stbi__jpeg_reset the entropy decoder and
// the dc prediction
static void stbi__jpeg_reset(struct jpeg *j)
{
   j->code_bits = 0;
   j->code_buffer = 0;
   j->nomore = 0;
   j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = j->img_comp[3].dc_pred = 0;
   j->marker = STBI__MARKER_none;
   j->eob_run = 0;
   // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
   // since we don't even allow 1<<30 pixels
}

static int stbi__parse_entropy_coded_data(struct jpeg *self)
{
   stbi__jpeg_reset(self);
   // interleaved
   int i,j,k,x,y;
   short data[64];
   for (j=0; j < self->img_mcu_y; ++j) {
      for (i=0; i < self->img_mcu_x; ++i) {
         // scan an interleaved mcu... process scan_n components in order
         for (k=0; k < self->scan_n; ++k) {
            int n = self->order[k];
            // scan out an mcu's worth of this component; that's just determined
            // by the basic H and V specified for the component
            for (y=0; y < self->img_comp[n].v; ++y) {
               for (x=0; x < self->img_comp[n].h; ++x) {
                  int x2 = (i*self->img_comp[n].h + x)*8;
                  int y2 = (j*self->img_comp[n].v + y)*8;
                  int ha = self->img_comp[n].ha;
                  if (!stbi__jpeg_decode_block(self, data, self->huff_dc+self->img_comp[n].hd, self->huff_ac+ha, self->fast_ac[ha], n, self->dequant[self->img_comp[n].tq])) return 0;
                  stbi__idct_block(self->img_comp[n].data+self->img_comp[n].w2*y2+x2, self->img_comp[n].w2, data);
               }
            }
         }
      }
   }
   return 1;
}

static int stbi__process_marker(struct jpeg *z, int m)
{
   int L;
   switch (m) {
      case STBI__MARKER_none: // no marker found
         return stbi__err("expected marker","Corrupt JPEG");

      case 0xDB: // DQT - define quantization table
         L = stbi__get16be(z)-2;
         while (L > 0) {
            int q = stbi__get8(z);
            int p = q >> 4, sixteen = (p != 0);
            int t = q & 15,i;
            if (p != 0 && p != 1) return stbi__err("bad DQT type","Corrupt JPEG");
            if (t > 3) return stbi__err("bad DQT table","Corrupt JPEG");

            for (i=0; i < 64; ++i)
               z->dequant[t][stbi__jpeg_dezigzag[i]] = (uint16_t)(sixteen ? stbi__get16be(z) : stbi__get8(z));
            L -= (sixteen ? 129 : 65);
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = stbi__get16be(z)-2;
         while (L > 0) {
            uint8_t *v;
            int sizes[16],i,n=0;
            int q = stbi__get8(z);
            int tc = q >> 4;
            int th = q & 15;
            if (tc > 1 || th > 3) return stbi__err("bad DHT header","Corrupt JPEG");
            for (i=0; i < 16; ++i) {
               sizes[i] = stbi__get8(z);
               n += sizes[i];
            }
            if(n > 256) return stbi__err("bad DHT header","Corrupt JPEG"); // Loop over i < n would write past end of values!
            L -= 17;
            if (tc == 0) {
               if (!stbi__build_huffman(z->huff_dc+th, sizes)) return 0;
               v = z->huff_dc[th].values;
            } else {
               if (!stbi__build_huffman(z->huff_ac+th, sizes)) return 0;
               v = z->huff_ac[th].values;
            }
            for (i=0; i < n; ++i)
               v[i] = stbi__get8(z);
            if (tc != 0)
               stbi__build_fast_ac(z->fast_ac[th], z->huff_ac + th);
            L -= n;
         }
         return L==0;
   }

   if (m == 0xE0) {
      L = stbi__get16be(z);
      if (L < 2) {
         return stbi__err("bad APP len","Corrupt JPEG");
      }
      L -= 2;

      stbi__skip(z, L);
      return 1;
   }

   return stbi__err("unknown marker","Corrupt JPEG");
}

// after we see SOS
static int stbi__process_scan_header(struct jpeg *z)
{
   int i;
   int Ls = stbi__get16be(z);
   z->scan_n = stbi__get8(z);
   // TODO:
   if (z->scan_n != 3) return stbi__err("bad SOS component count","Corrupt JPEG");
   // if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int) z->s->img_n) return stbi__err("bad SOS component count","Corrupt JPEG");
   if (Ls != 6+2*z->scan_n) return stbi__err("bad SOS len","Corrupt JPEG");
   for (i=0; i < z->scan_n; ++i) {
      int id = stbi__get8(z), which;
      int q = stbi__get8(z);
      for (which = 0; which < jpeg_IMG_N_COMP; ++which)
         if (z->img_comp[which].id == id)
            break;
      if (which == jpeg_IMG_N_COMP) return 0; // no match TODO
      z->img_comp[which].hd = q >> 4;   if (z->img_comp[which].hd > 3) return stbi__err("bad DC huff","Corrupt JPEG");
      z->img_comp[which].ha = q & 15;   if (z->img_comp[which].ha > 3) return stbi__err("bad AC huff","Corrupt JPEG");
      z->order[i] = which;
   }

   {
      int aa;
      int spec_start = stbi__get8(z);
      int spec_end   = stbi__get8(z); // should be 63, but might be 0
      aa = stbi__get8(z);
      int succ_high = (aa >> 4);
      int succ_low  = (aa & 15);
      if (spec_start != 0) return stbi__err("bad SOS","Corrupt JPEG");
      if (succ_high != 0 || succ_low != 0) return stbi__err("bad SOS","Corrupt JPEG");
   }

   return 1;
}

static int stbi__process_frame_header(struct jpeg *z) {
   int Lf,p,i,q, h_max=1,v_max=1,c;
   Lf = stbi__get16be(z);         if (Lf < 11) return stbi__err("bad SOF len","Corrupt JPEG"); // JPEG
   p  = stbi__get8(z);            if (p != 8) return stbi__err("only 8-bit","JPEG format not supported: 8-bit only"); // JPEG baseline
   z->img_y = stbi__get16be(z);   if (z->img_y == 0) return stbi__err("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
   z->img_x = stbi__get16be(z);   if (z->img_x == 0) return stbi__err("0 width","Corrupt JPEG"); // JPEG requires
   if (z->img_y > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
   if (z->img_x > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
   c = stbi__get8(z);
   if (c != jpeg_IMG_N_COMP) return stbi__err("bad component count","Corrupt JPEG");
   for (i=0; i < c; ++i) {
      z->img_comp[i].data = NULL;
      z->img_comp[i].linebuf = NULL;
   }

   if (Lf != 8+3*jpeg_IMG_N_COMP) return stbi__err("bad SOF len","Corrupt JPEG");

   for (i=0; i < jpeg_IMG_N_COMP; ++i) {
      z->img_comp[i].id = stbi__get8(z);
      q = stbi__get8(z);
      z->img_comp[i].h = (q >> 4);  if (!z->img_comp[i].h || z->img_comp[i].h > 4) return stbi__err("bad H","Corrupt JPEG");
      z->img_comp[i].v = q & 15;    if (!z->img_comp[i].v || z->img_comp[i].v > 4) return stbi__err("bad V","Corrupt JPEG");
      z->img_comp[i].tq = stbi__get8(z);  if (z->img_comp[i].tq > 3) return stbi__err("bad TQ","Corrupt JPEG");
   }

   for (i=0; i < jpeg_IMG_N_COMP; ++i) {
      if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
      if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
   }

   // check that plane subsampling factors are integer ratios; our resamplers can't deal with fractional ratios
   // and I've never seen a non-corrupted JPEG file actually use them
   for (i=0; i < jpeg_IMG_N_COMP; ++i) {
      if (h_max % z->img_comp[i].h != 0) return stbi__err("bad H","Corrupt JPEG");
      if (v_max % z->img_comp[i].v != 0) return stbi__err("bad V","Corrupt JPEG");
   }

   // compute interleaved mcu info
   z->img_h_max = h_max;
   z->img_v_max = v_max;
   z->img_mcu_w = h_max * 8;
   z->img_mcu_h = v_max * 8;
   // these sizes can't be more than 17 bits
   z->img_mcu_x = (z->img_x + z->img_mcu_w-1) / z->img_mcu_w;
   z->img_mcu_y = (z->img_y + z->img_mcu_h-1) / z->img_mcu_h;

   for (i=0; i < jpeg_IMG_N_COMP; ++i) {
      // number of effective pixels (e.g. for non-interleaved MCU)
      z->img_comp[i].x = (z->img_x * z->img_comp[i].h + h_max-1) / h_max;
      z->img_comp[i].y = (z->img_y * z->img_comp[i].v + v_max-1) / v_max;
      // to simplify generation, we'll allocate enough memory to decode
      // the bogus oversized data from using interleaved MCUs and their
      // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
      // discard the extra data until colorspace conversion
      //
      // img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
      // so these muls can't overflow with 32-bit ints (which we require)
      z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
      z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
   }

   return 1;
}

// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define stbi__SOI(x)         ((x) == 0xd8)
#define stbi__EOI(x)         ((x) == 0xd9)
#define stbi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
#define stbi__SOS(x)         ((x) == 0xda)

static int stbi__decode_jpeg_header(struct jpeg *z)
{
   int m;
   z->marker = STBI__MARKER_none; // initialize cached marker to empty
   m = stbi__get_marker(z);
   if (!stbi__SOI(m)) return stbi__err("no SOI","Corrupt JPEG");
   m = stbi__get_marker(z);
   while (!stbi__SOF(m)) {
      if (!stbi__process_marker(z,m)) return 0;
      m = stbi__get_marker(z);
   }
   if (!stbi__process_frame_header(z)) return 0;
   return 1;
}

// decode image to YCbCr format
static int stbi__decode_jpeg_image(struct jpeg *j)
{
   int m = stbi__get_marker(j);
   while (!stbi__EOI(m)) {
      if (stbi__SOS(m)) {
         if (!stbi__process_scan_header(j)) return 0;
         if (!stbi__parse_entropy_coded_data(j)) return 0;
         m = stbi__get_marker(j);
      } else {
         if (!stbi__process_marker(j, m)) return 1;
         m = stbi__get_marker(j);
      }
   }
   return 1;
}

// static jfif-centered resampling (across block boundaries)

typedef uint8_t *(*resample_row_func)(uint8_t *out, uint8_t *in0, uint8_t *in1,
                                    int w, int hs);

#define stbi__div4(x) ((uint8_t) ((x) >> 2))

static uint8_t *resample_row_1(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   (void)out;
   (void)in_far;
   (void)w;
   (void)hs;
   return in_near;
}

static uint8_t* stbi__resample_row_v_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   (void)hs;

   // need to generate two samples vertically for every one in input
   int i;
   for (i=0; i < w; ++i)
      out[i] = stbi__div4(3*in_near[i] + in_far[i] + 2);
   return out;
}

static uint8_t*  stbi__resample_row_h_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   (void)in_far;
   (void)hs;

   // need to generate two samples horizontally for every one in input
   int i;
   uint8_t *input = in_near;

   if (w == 1) {
      // if only one sample, can't do any interpolation
      out[0] = out[1] = input[0];
      return out;
   }

   out[0] = input[0];
   out[1] = stbi__div4(input[0]*3 + input[1] + 2);
   for (i=1; i < w-1; ++i) {
      int n = 3*input[i]+2;
      out[i*2+0] = stbi__div4(n+input[i-1]);
      out[i*2+1] = stbi__div4(n+input[i+1]);
   }
   out[i*2+0] = stbi__div4(input[w-2]*3 + input[w-1] + 2);
   out[i*2+1] = input[w-1];

   return out;
}

#define stbi__div16(x) ((uint8_t) ((x) >> 4))

static uint8_t *stbi__resample_row_hv_2(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   (void)hs;

   // need to generate 2x2 samples for every one in input
   int i,t0,t1;
   if (w == 1) {
      out[0] = out[1] = stbi__div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   t1 = 3*in_near[0] + in_far[0];
   out[0] = stbi__div4(t1+2);
   for (i=1; i < w; ++i) {
      t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = stbi__div16(3*t0 + t1 + 8);
      out[i*2  ] = stbi__div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = stbi__div4(t1+2);

   return out;
}

static uint8_t *stbi__resample_row_generic(uint8_t *out, uint8_t *in_near, uint8_t *in_far, int w, int hs)
{
   (void)in_far;

   // resample with nearest-neighbor
   int i,j;
   for (i=0; i < w; ++i)
      for (j=0; j < hs; ++j)
         out[i*hs+j] = in_near[i];
   return out;
}

// this is a reduced-precision calculation of YCbCr-to-RGB introduced
// to make sure the code produces the same results in both SIMD and scalar
#define stbi__float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
static void stbi__YCbCr_to_RGB_row(int32_t *out, const uint8_t *y, const uint8_t *pcb, const uint8_t *pcr, int32_t count)
{
   for (int32_t i = 0; i < count; ++i) {
      uint32_t y_fixed = (y[i] << 20) + (1<<19); // rounding
      uint32_t cr = pcr[i] - 128;
      uint32_t cb = pcb[i] - 128;
      uint32_t r = y_fixed +  cr* stbi__float2fixed(1.40200f);
      uint32_t g = y_fixed + (cr*-stbi__float2fixed(0.71414f)) + ((cb*-stbi__float2fixed(0.34414f)) & 0xffff0000);
      uint32_t b = y_fixed                                     +   cb* stbi__float2fixed(1.77200f);
      *out++ = (int32_t)((uint32_t)0xFF000000 + ((r & 0x0FF00000) >> 4) + ((g & 0x0FF00000) >> 12) + ((b & 0x0FF00000) >> 20));
   }
}

struct jpeg_resample {
   resample_row_func resample;
   uint8_t *line0,*line1;
   int hs,vs;   // expansion factor in each axis
   int w_lores; // horizontal pixels pre-expansion
   int ystep;   // how far through vertical expansion we are
   int ypos;    // which pre-expansion row we're on
};

int32_t *jpeg_decode(struct jpeg *self, uint8_t *buffer, int32_t buffer_length, int32_t *out_width, int32_t *out_height) {
   self->img_buffer = buffer;
   self->img_buffer_end = &buffer[buffer_length];

   if (!stbi__decode_jpeg_header(self)) platform_ABORT();

   *out_width = self->img_x;
   *out_height = self->img_y;

   if (!stbi__mul3sizes_valid(self->img_x, self->img_y, 4)) platform_ABORT();
   int32_t *output = platform_heap_alloc(self->img_x * self->img_y, 4);

   struct jpeg_resample res_comp[4];
   for (int k=0; k < jpeg_IMG_N_COMP; ++k) {
      if (!stbi__mul2sizes_valid(self->img_comp[k].w2, self->img_comp[k].h2)) platform_ABORT();
      self->img_comp[k].data = platform_heap_alloc(self->img_comp[k].w2 * self->img_comp[k].h2, 1);

      struct jpeg_resample *r = &res_comp[k];

      // allocate line buffer big enough for upsampling off the edges
      // with upsample factor of 4
      self->img_comp[k].linebuf = platform_heap_alloc(self->img_x + 3, 1);

      r->hs      = self->img_h_max / self->img_comp[k].h;
      r->vs      = self->img_v_max / self->img_comp[k].v;
      r->ystep   = r->vs >> 1;
      r->w_lores = (self->img_x + r->hs-1) / r->hs;
      r->ypos    = 0;
      r->line0   = r->line1 = self->img_comp[k].data;

      if      (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
      else if (r->hs == 1 && r->vs == 2) r->resample = stbi__resample_row_v_2;
      else if (r->hs == 2 && r->vs == 1) r->resample = stbi__resample_row_h_2;
      else if (r->hs == 2 && r->vs == 2) r->resample = stbi__resample_row_hv_2;
      else                               r->resample = stbi__resample_row_generic;
   }

   // load a jpeg image from whichever source, but leave in YCbCr format
   if (!stbi__decode_jpeg_image(self)) platform_ABORT();

   // resample and color-convert
   unsigned int j;
   uint8_t *coutput[jpeg_IMG_N_COMP];

   // now go ahead and resample
   for (j=0; j < self->img_y; ++j) {
      int32_t *out = &output[self->img_x * j];
      for (int k = 0; k < jpeg_IMG_N_COMP; ++k) {
         struct jpeg_resample *r = &res_comp[k];
         int y_bot = r->ystep >= (r->vs >> 1);
         coutput[k] = r->resample(self->img_comp[k].linebuf,
                                    y_bot ? r->line1 : r->line0,
                                    y_bot ? r->line0 : r->line1,
                                    r->w_lores, r->hs);
         if (++r->ystep >= r->vs) {
            r->ystep = 0;
            r->line0 = r->line1;
            if (++r->ypos < self->img_comp[k].y)
               r->line1 += self->img_comp[k].w2;
         }
      }
      stbi__YCbCr_to_RGB_row(out, coutput[0], coutput[1], coutput[2], self->img_x);
   }

   platform_heap_reset(&output[self->img_x * self->img_y]);
   return output;
}

