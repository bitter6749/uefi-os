#ifndef IO_H
#define IO_H

// ====================================================================================
// 8-bit I/O ポート操作 (キーボード、PIC等)
// ====================================================================================
static inline unsigned char inb(unsigned short port) {
  unsigned char ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outb(unsigned short port, unsigned char data) {
  __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// ====================================================================================
// 32-bit I/O ポート操作 (PCI設定空間等)
// ====================================================================================
static inline unsigned int inl(unsigned short port) {
  unsigned int ret;
  __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outl(unsigned short port, unsigned int data) {
  __asm__ volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

#endif
