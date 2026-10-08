#include "drivers/console.h"
#include "drivers/font.h"
#include "drivers/graphics.h"

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
  // -----------------------------------------------------------------------
  // 1. バックスペース ('\b': 0x08) の処理
  // -----------------------------------------------------------------------
  if (c == '\b') {
    // これ以上左に戻れない場合 (行頭) の制御
    if (con->cursor_x >= FONT_WIDTH) {
      con->cursor_x -= FONT_WIDTH;
    } else if (con->cursor_y >= FONT_HEIGHT) {
      // 前の行の右端に戻す場合 (コンソールの横幅に合わせて調整)
      con->cursor_y -= FONT_HEIGHT;
      con->cursor_x = (con->fb->width / FONT_WIDTH - 1) * FONT_WIDTH;
    } else {
      return; // 画面最左上なら何もしない
    }

    // カーソル位置の 1 文字分を背景色で塗りつぶして消去
    draw_rect(con->fb, con->cursor_x, con->cursor_y, FONT_WIDTH, FONT_HEIGHT, con->bg_color);
    return;
  }

  // -----------------------------------------------------------------------
  // 2. 改行コード ('\n') の処理
  // -----------------------------------------------------------------------
  if (c == '\n') {
    con->cursor_x = 0;    // 左端に戻す
    con->cursor_y += FONT_HEIGHT;  // 1行下へ進める

    // 画面下端を超えた場合のスクロール処理
    if (con->cursor_y + FONT_HEIGHT > con->fb->height) {
      console_scroll(con);
      con->cursor_y = con->fb->height - FONT_HEIGHT; // 最下行にカーソルを固定
    }
    return;
  }

  // -----------------------------------------------------------------------
  // 3. 通常文字の描画
  // -----------------------------------------------------------------------
  draw_char(con->fb, con->cursor_x, con->cursor_y, c, con->fg_color);
  con->cursor_x += FONT_WIDTH;       // 次の文字位置へ (横8ドット進める)

  // 右端に達したら自動折返し
  if (con->cursor_x + FONT_WIDTH > con->fb->width) {
    con->cursor_x = 0;
    con->cursor_y += FONT_HEIGHT;
  }

  // 4. 画面下端を超えた場合のスクロール処理
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

// =========================================================================
// console_put_hex: コンソールに 16 進数文字列を出力する
// =========================================================================
void console_put_hex(Console *con, unsigned long long val, int digits) {
  console_puts(con, "0x");
  for (int i = digits - 1; i >= 0; i--) {
    // 4 ビット ずつ取り出す
    unsigned char nibble = (val >> (i * 4)) & 0x0F;
    if (nibble < 10) {
      console_putc(con, '0' + nibble);
    } else {
      console_putc(con, 'A' + (nibble - 10));
    }
  } 
}

// =========================================================================
// console_put_hex: コンソールに 16 進数文字列を出力する
// =========================================================================
void console_put_dec(Console *con, unsigned long long val) {
  if (val == 0) {
    console_putc(con, '0');
    return;
  }

  char buf[24];
  int i = 0;

  // 1の位から順に取り出してバッファに積む
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }

  // 逆順 (上位の桁から順) に出力
  while (--i >= 0) {
    console_putc(con, buf[i]);
  }
}

// ============================================================================
// console_clear: コンソール画面全体をクリアし、カーソルを初期位置 (0, 0) に戻す
// ============================================================================
void console_clear(Console *con) {
  if (con == 0 || con->fb == 0) {
    return;
  }

  // 画面全体を背景色で塗りつぶす
  draw_rect(con->fb, 0, 0, con->fb->width, con->fb->height, con->bg_color);

  // カーソル一を原点にリセット
  con->cursor_x = 0;
  con->cursor_y = 0;
}
















