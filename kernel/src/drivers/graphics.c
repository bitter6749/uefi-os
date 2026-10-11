#include "drivers/graphics.h"

void draw_pixel(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int color) {
  // 画面境界外への不正メモリアクセスを防ぐ (保護ガード)
  if (x >= fb->width || y >= fb->height) {
    return;
  }

  fb->base[y * fb->ppsl + x] = color;
}

void draw_rect(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color) {
  for (unsigned int dy = 0; dy < h; dy++) {
    for (unsigned int dx = 0; dx < w; dx++) {
      draw_pixel(fb, x + dx, y + dy, color);
    }
  }
}

void clear_screen(FrameBuffer *fb, unsigned int color) {
  draw_rect(fb, 0, 0, fb->width, fb->height, color);
}
