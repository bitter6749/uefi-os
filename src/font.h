#ifndef FONT_H
#define FONT_H

#include "graphics.h"

// ============================================================================
// draw_char: ビットマップフォントを用いて画面上に 1 文字を描画する
// ============================================================================
// - 引数1 (fb):      描画対象のフレームバッファ情報へのポインタ
// - 引数2 (x):       文字の左上端となる X 座標
// - 引数3 (y):       文字の左上端となる Y 座標
// - 引数4 (c):       描画対象の ASCII 文字 (例: 'A')
// - 引数5 (color):   描画する文字の色 (32-bit RGB: 0x00RRGGBB)
void draw_char(FrameBuffer *fb, unsigned int x, unsigned y, char c, unsigned int color);

#endif
