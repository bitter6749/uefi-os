#include "drivers/graphics.h"

// 8バイト (64bit)単位で一括転送する関数
static inline void fast_copy_64(void *dst, const void *src, unsigned long long qwords) {
  __asm__ volatile (
      "rep movsq"
      : "+D"(dst), "+S"(src), "+c"(qwords)
      :
      : "memory"
  );
}

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

  if (fb->width == fb->ppsl) {
    unsigned long long total_qwords = ((unsigned long long)fb->ppsl * fb->height) / 2;
    fast_copy_64(fb->vram, fb->back_buffer, total_qwords);
  } else {
    unsigned long long row_qwords = fb->width / 2;
    for (unsigned int y = 0; y < fb->height; y++) {
      fast_copy_64(&fb->vram[y * fb->ppsl], &fb->back_buffer[y * fb->ppsl], row_qwords);
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
