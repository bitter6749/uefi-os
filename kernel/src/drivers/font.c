#include "drivers/font.h"
#include "drivers/graphics.h"

// objcopy によってリンクされたフォントバイナリの先頭シンボル
extern const unsigned char _binary_assets_font_bin_start[];

void draw_char(FrameBuffer *fb, unsigned int x, unsigned int y, char c, unsigned int color) {
  const unsigned char *font = &_binary_assets_font_bin_start[(unsigned char)c * FONT_HEIGHT];

  for (int dy = 0; dy < FONT_HEIGHT; dy++) {
    unsigned char line = font[dy];
    for (int dx = 0; dx < FONT_WIDTH; dx++) {
      // line を左に dx ビットシフトし、最上位ビット (0x80 = 10000000b) が立っているか判定
      if ((line << dx) & 0x80) {
        draw_pixel(fb, x + dx, y + dy, color);
      }
    }
  }
}
