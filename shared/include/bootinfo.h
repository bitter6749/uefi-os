// shared/bootinfo.h
#ifndef BOOTINFO_H
#define BOOTINFO_H

typedef struct {
  void *base;
  unsigned int width;
  unsigned int height;
  unsigned int ppsl;
} BootFrameBuffer;

// =====================================================================
// カーネル側で保持するメモリマップ情報構造体
// =====================================================================
typedef struct {
  unsigned long long  buffer_size;          // バッファ全体のバイトサイズ
  void                *buffer;              // メモリ記述子配列へのポインタ
  unsigned long long  map_size;             // 実際に取得されたメモリマップのバイトサイズ
  unsigned long long  map_key;              // ExitBootServices で必要な識別キー
  unsigned long long  descriptor_size;      // 1つの記述子のバイトサイズ
  unsigned int        descriptor_version;   // 記述子のバージョン
} BootMemoryMap;

typedef struct {
  BootFrameBuffer fb;
  BootMemoryMap map;
  void *runtime_services; // EFI_RUNTIME_SERVICES ポインタ
} BootInfo;

#endif
