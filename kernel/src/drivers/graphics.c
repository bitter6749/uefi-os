#include "drivers/graphics.h"

void draw_pixel(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int color) {
  // 画面境界外への不正メモリアクセスを防ぐ (保護ガード)
  if (x >= fb->width || y >= fb->height) {
    return;
  }

  unsigned int *target = fb->back_buffer ? fb->back_buffer : fb->vram;
  target[y * fb->ppsl + x] = color;
}

void draw_rect(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color) {
  if (x >= fb->width || y >= fb->height) {
    return;
  }

  if (x + w > fb->width)  w = fb->width - x;
  if (y + h > fb->height) h = fb->height - y;

  unsigned int *target = fb->back_buffer ? fb->back_buffer : fb->vram;
  for (unsigned int dy = 0; dy < h; dy++) {
    unsigned int *line = &target[(y + dy) * fb->ppsl + x];
    for (unsigned int dx = 0; dx < w; dx++) {
      line[dx] = color;
    }
  }
}

void clear_screen(FrameBuffer *fb, unsigned int color) {
  draw_rect(fb, 0, 0, fb->width, fb->height, color);
}

void graphics_flush(FrameBuffer *fb) {
  if (!fb->back_buffer || !fb->vram) {
    return;
  }

  for (unsigned int y = 0; y < fb->height; y++) {
    unsigned int *dst = &fb->vram[y * fb->ppsl];
    unsigned int *src = &fb->back_buffer[y * fb->ppsl];
    for (unsigned int x = 0; x < fb->width; x++) {
      dst[x] = src[x];
    }
  }
}

void graphics_flush_rect(FrameBuffer *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
  if (!fb->back_buffer || !fb->vram) {
    return;
  }
  if (x >= fb->width || y >= fb->height) {
    return;
  }

  if (x + w > fb->width)  w = fb->width - x;
  if (y + h > fb->height) h = fb->height - y;

  for (unsigned int dy = 0; dy < h; dy++) {
    unsigned int *dst = &fb->vram[(y + dy) * fb->ppsl + x];
    unsigned int *src = &fb->back_buffer[(y + dy) * fb->ppsl + x];
    for (unsigned int dx = 0; dx < w; dx++) {
      dst[dx] = src[dx];
    }
  }
}
