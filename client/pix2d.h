
extern int32_t *pix2d_data;
extern int32_t pix2d_width;
extern int32_t pix2d_height;
extern int32_t pix2d_top;
extern int32_t pix2d_bottom;
extern int32_t pix2d_left;
extern int32_t pix2d_right;
extern int32_t pix2d_safe_width;
extern int32_t pix2d_center_x;
extern int32_t pix2d_center_y;

void pix2d_bind(int32_t width, int32_t *data, int32_t height);
void pix2d_reset_bounds(void);
void pix2d_set_bounds(int32_t right, int32_t bottom, int32_t top, int32_t left);
void pix2d_clear(void);
void pix2d_fill_rect_trans(int32_t y, int32_t alpha, int32_t height, int32_t width, int32_t colour, int32_t x);
void pix2d_fill_rect(int32_t colour, int32_t width, int32_t height, int32_t x, int32_t y);
void pix2d_draw_rect(int32_t height, int32_t width, int32_t colour, int32_t x, int32_t y);
void pix2d_draw_rect_trans(int32_t height, int32_t colour, int32_t x, int32_t y, int32_t width, int32_t alpha);
void pix2d_draw_horisontal_line(int32_t colour, int32_t y, int32_t width, int32_t x);
void pix2d_draw_horisontal_line_trans(int32_t y, int32_t width, int32_t colour, int32_t x, int32_t alpha);
void pix2d_draw_vertical_line(int32_t x, int32_t colour, int32_t y, int32_t height);
void pix2d_draw_vertical_line_trans(int32_t x, int32_t y, int32_t alpha, int32_t height, int32_t colour);
