#include "font.h"
#include "graphics.h"

// 'A' の 8x16 ビットマップフォントデータ
// ビットが 1 の箇所に色を配置し、0 の箇所は描画しない
static const unsigned char font_A[16] = {
  0x00, // 00000000
  0x00, // 00000000
  0x10, // 00010000
  0x38, // 00111000
  0x6C, // 01101100
  0xC6, // 11000110
  0xC6, // 11000110
  0xFE, // 11111110
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
};

void draw_char(FrameBuffer *fb, unsigned int x, unsigned int y, char c, unsigned int color) {
  const unsigned char *font = 0;

  // 実験用として、現在は 'A' のみ判定
  if (c == 'A') {
    font = font_A;
  } else {
    return; // 未対応文字は描画せずスキップ
  }

  for (int dy = 0; dy < 16; dy++) {
    unsigned char line = font[dy];
    for (int dx = 0; dx < 8; dx++) {
      // line を左に dx ビットシフトし、最上位ビット (0x80 = 10000000b) が立っているか判定
      if ((line << dx) & 0x80) {
        draw_pixel(fb, x + dx, y + dy, color);
      }
    }
  }
}
