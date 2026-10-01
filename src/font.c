#include "font.h"
#include "graphics.h"

// 8x16 ビットマップフォントデータ
// ビットが 1 の箇所に色を配置し、0 の箇所は描画しない

// 文字 'A'
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

// 文字 'H'
static const unsigned char font_H[16] = {
  0x00, // 00000000
  0x00, // 00000000
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0xFE, // 11111110
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
};

// 文字 'E'
static const unsigned char font_E[16] = {
  0x00, // 00000000
  0x00, // 00000000
  0xFE, // 11111110
  0xC0, // 11000000
  0xC0, // 11000000
  0xFC, // 11111100
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xFE, // 11111110
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
};

// 文字 'L'
static const unsigned char font_L[16] = {
  0x00, // 00000000
  0x00, // 00000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xC0, // 11000000
  0xFE, // 11111110
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
};

// 文字 'O'
static const unsigned char font_O[16] = {
  0x00, // 00000000
  0x00, // 00000000
  0x38, // 00111000
  0x6C, // 01101100
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0xC6, // 11000110
  0x6C, // 01101100
  0x38, // 00111000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
};

// 空白 (スペース ' ')
static const unsigned char font_space[16] = {
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
};

// 未対応文字用の四角枠
static const unsigned char font_undefined[16] = {
  0x00, // 00000000
  0x00, // 00000000
  0xFE, // 11111110
  0x82, // 10000010
  0x82, // 10000010
  0x82, // 10000010
  0x82, // 10000010
  0x82, // 10000010
  0x82, // 10000010
  0x82, // 10000010
  0xFE, // 11111110
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
  0x00, // 00000000
};

// 指定された文字に対応する 16 バイトのフォントデータを取得する
static const unsigned char *get_font(char c) {
  switch (c) {
    case 'A': return font_A;
    case 'H': return font_H;
    case 'E': return font_E;
    case 'L': return font_L;
    case 'O': return font_O;
    case ' ': return font_space;
    default: return font_undefined;
  }
}

void draw_char(FrameBuffer *fb, unsigned int x, unsigned int y, char c, unsigned int color) {
  const unsigned char *font = get_font(c);

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
