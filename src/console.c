#include "console.h"
#include "font.h"
#include "graphics.h"

// =========================================================================
// console_init: コンソールを初期化する
// =========================================================================
void console_init(Console *con, FrameBuffer *fb, unsigned int fg_color, unsigned int bg_color) {
  con->fb       = fb;
  con->cursor_x = 0;
  con->cursor_y = 0;
  con->fg_color = fg_color;
  con->bg_color = bg_color;
}

// =========================================================================
// console_scroll: 画面全体を 1 行分 (16 ピクセル) 上にスクロールする
// =========================================================================
void console_scroll(Console *con) {
  FrameBuffer *fb = con->fb;

  for (unsigned int y = FONT_HEIGHT; y < fb->height; y++) {
    for (unsigned int x = 0; x < fb->width; x++) {
      fb->base[(y - FONT_HEIGHT) * fb->ppsl + x] = fb->base[y * fb->ppsl + x];
    }
  }

  // 最下部に空いた高さ 16px の領域を背景色で塗りつぶす
  draw_rect(fb, 0, fb->height - FONT_HEIGHT, fb->width, FONT_HEIGHT, con->bg_color);
}

// =========================================================================
// console_putc: コンソールに文字を 1 文字出力する
// =========================================================================
void console_putc(Console *con, char c) {
  // 1. 改行コード ('\n') の処理
  if (c == '\n') {
    con->cursor_x = 0;    // 左端に戻す
    con->cursor_y += FONT_HEIGHT;  // 1行下へ進める
  } else {
    // 2. 通常文字の描画
    draw_char(con->fb, con->cursor_x, con->cursor_y, c, con->fg_color);
    con->cursor_x += FONT_WIDTH;       // 次の文字位置へ (横8ドット進める)

    // 右端に達したら自動折返し
    if (con->cursor_x + FONT_WIDTH > con->fb->width) {
      con->cursor_x = 0;
      con->cursor_y += FONT_HEIGHT;
    }
  }

  // 3. 画面下端を超えた場合のスクロール処理
  if (con->cursor_y + FONT_HEIGHT > con->fb->height) {
    console_scroll(con);
    con->cursor_y = con->fb->height - FONT_HEIGHT; // 最下行にカーソルを固定
  }
}

// =========================================================================
// console_puts: コンソールに NULL 終端文字列を出力する
// =========================================================================
void console_puts(Console *con, const char *str) {
  while (*str) {
    console_putc(con, *str);
    str++;
  }
}

