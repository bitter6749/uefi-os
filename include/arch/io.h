#ifndef IO_H
#define IO_H

// ====================================================================================
// inb: 指定した I/O ポートから 1 バイト読み込む
// ====================================================================================
static inline unsigned char inb(unsigned short port) {
  unsigned char ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

// ====================================================================================
// outb: 指定した I/O ポートへ 1 バイト書き込む
// ====================================================================================
static inline void outb(unsigned short port, unsigned char data) {
  __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

#endif
