#ifndef GRAPHICS_H
#define GRAPHICS_H

// 画面 (フレームバッファ) の情報
typedef struct {
  unsigned int *base;   // VRAM の先頭アドレス
  unsigned int width;   // 画面の横幅 (px)
  unsigned int height;  // 画面の縦幅 (px)
  unsigned int ppsl;    // 1行あたりのピクセル数
} FrameBuffer;

// ============================================================================
// draw_pixel: 画面上の指定座標 (x, y) に 1 ピクセルを描画する
// ============================================================================
// - 引数1 (fb):      描画対象のフレームバッファ情報へのポインタ
// - 引数2 (x):       描画するピクセルの X 座標 (0 <= x <= width)
// - 引数3 (y):       描画するピクセルの Y 座標 (0 <= y <= height)
// - 引数4 (color):   描画する色 (32-bit RGB: 0x00RRGGBB)
void draw_pixel(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int color);

// ============================================================================
// draw_rect: 指定位置に塗りつぶしの矩形 (四角形) を描画する
// ============================================================================
// - 引数1 (fb):      描画対象のフレームバッファ情報へのポインタ
// - 引数2 (x):       矩形の左上端の X 座標
// - 引数3 (y):       矩形の左上端の Y 座標
// - 引数4 (w):       矩形の横幅 (ピクセル)
// - 引数5 (h):       矩形の縦幅 (ピクセル)
// - 引数6 (color):   描画する色 (32-bit RGB: 0x00RRGGBB)
void draw_rect(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color);

// ============================================================================
// clear_screen: 画面全体を指定した色で塗りつぶす (画面クリア)
// ============================================================================
// - 引数1 (fb):      描画対象のフレームバッファ情報へのポインタ
// - 引数2 (color):   画面全体の背景色 (32-bit RGB: 0x00RRGGBB)
void clear_screen(FrameBuffer *fb, unsigned int color);

#endif
