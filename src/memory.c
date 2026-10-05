#include "memory.h"
#include "efi.h"

// ビットマップ操作ヘルパー関数 (静的関数)

// ====================================================================================
// bitmap_set: 指定したページインデックスのビットを 1 (使用中: FRAME_USED) に設定する
// ====================================================================================
// - 引数1 (bitmap):      物理ページフレームアロケータ構造体へのポインタ
// - 引数2 (page_index):  取得したいページインデックス
static inline void bitmap_set(unsigned char *bitmap, unsigned long long page_index) {
  // 1. page_index / BITS_PER_BYTE (8):
  //    対象のビットが「bitmap 配列の何バイト目の要素か (バイトインデックス)」を算出する。
  // 2. page_index % BITS_PER_BYTE (8):
  //    対象バイト内の「何番目のビットか (0~7)」を算出する。
  // 3. (1 << (page_index % BITS_PER_BYTE (8))):
  //    対象ビットの位置だけを 1 にしたビットマスクを作成する (例: 3番目なら 00001000b)
  // 4. |= (OR演算):
  //    既存のバイト値に対してマスクを OR 演算し、対象ビットのみを 1 に変更 (他のビットは不変)
  bitmap[page_index / BITS_PER_BYTE] |= (1 << (page_index % BITS_PER_BYTE));
}

// ====================================================================================
// bitmap_clear: 指定したページインデックスのビットを 0 (空き: FRAME_FREE) に設定する
// ====================================================================================
// - 引数1 (bitmap):      物理ページフレームアロケータ構造体へのポインタ
// - 引数2 (page_index):  解放したいページインデックス
static inline void bitmap_clear(unsigned char *bitmap, unsigned long long page_index) {
  // 1. (1 << (page_index % BITS_PER_BYTE (8))):
  //    対象ビットのみが 1 のビットマスクを作成する。(例: 2番目なら 00000100b)
  // 2. ~(ビット反転):
  //    マスクを反転させ、対象ビットのみ 0, 他をすべて 1 にする
  // 3. &= (AND演算):
  //    既存のバイト値と AND 演算し、対象ビットのみを強制的に 0 へ変更する (他のビットは不変)
  //
  // 例:  bitmap: 01100101b, 
  //      mask: 00000100b -> flip_mask: 11111011b
  //
  //                           元のbitmap
  //      01100101b             01100101b
  //          &           ->    01100001b
  //      11111011b           解放後のbitmap
  //
  bitmap[page_index / BITS_PER_BYTE] &= ~(1 << (page_index % BITS_PER_BYTE));
}

// ========================================================================================
// bitmap_get: 指定したページインデックスのビット状態 (FRAME_USED / FRAME_FREE) を取得する
// ========================================================================================
// - 引数1 (bitmap):      物理ページフレームアロケータ構造体へのポインタ
// - 引数2 (page_index):  状態を取得したいページインデックス
static inline int bitmap_get(const unsigned char *bitmap, unsigned long long page_index) {
  // 1. bitmap[page_index / BITS_PER_BYTE (8)] >> (page_index % BITS_PER_BYTE (8)):
  //    対象のビットが最右端 (LSB: 第0ビット) に位置するよう右シフトする。
  // 2. & 1:
  //    最右端以外の余分な上位ビットを削り落とし、0 か 1 の整数値として返す。
  return (bitmap[page_index / BITS_PER_BYTE] >> (page_index % BITS_PER_BYTE)) & 1;
}

// ====================================================================================
// frame_allocator_init: メモリマップをもとにフレームアロケータを初期化する
// ====================================================================================
void frame_allocator_init(BitmapFrameAllocator *allocator, MemoryMap *map) {
  unsigned long long max_phys_addr = 0;

  // 1. 全物理アドレスの最大物理アドレスを計算し、総ページ数を求める
  unsigned long long offset = 0;
  while (offset < map->map_size) {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((unsigned long long)map->buffer + offset);
    unsigned long long end_addr = desc->PhysicalStart + desc->NumberOfPages * PAGE_SIZE;
    if (end_addr > max_phys_addr) {
      max_phys_addr = end_addr;
    }
    offset += map->descriptor_size;
  }

  allocator->total_pages  = max_phys_addr / PAGE_SIZE;
  allocator->free_pages   = 0;

  // 2. ビットマップデータ自体を配置する領域を EfiConventionalMemory から検索
  // 必要サイズ: total_pages / 8 バイト
  unsigned long long bitmap_bytes = (allocator->total_pages + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE;
  allocator->bitmap = 0;

  offset = 0;
  while (offset < map->map_size) {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((unsigned long long)map->buffer + offset);
    if (desc->Type == EfiConventionalMemory && desc->NumberOfPages * PAGE_SIZE >= bitmap_bytes) {
      allocator->bitmap = (unsigned char *)desc->PhysicalStart;
      break;
    }
    offset += map->descriptor_size;
  }

  // 初期状態として全ページを「使用中 (1)」に設定
  for (unsigned long long i = 0; i < bitmap_bytes; i++) {
    allocator->bitmap[i] = 0xFF;
  }

  // 3. EfiConventionalMemory (空き領域) のページのみ「空き (0)」に書き換え
  offset = 0;
  while (offset < map->map_size) {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((unsigned long long)map->buffer + offset);
    if (desc->Type == EfiConventionalMemory) {
      unsigned long long start_page = desc->PhysicalStart / PAGE_SIZE;
      for (unsigned long long i = 0; i < desc->NumberOfPages; i++) {
        bitmap_clear(allocator->bitmap, start_page + i);
        allocator->free_pages++;
      }
    }
    offset += map->descriptor_size;
  }

  // 4. ビットマップ自体が配置されている領域は「使用中(1)」にして保護
  unsigned long long bitmap_start_page = (unsigned long long)allocator->bitmap / PAGE_SIZE;
  unsigned long long bitmap_page_count = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
  for (unsigned long long i = 0; i < bitmap_page_count; i++) {
    bitmap_set(allocator->bitmap, bitmap_start_page + i);
    if (allocator->free_pages > 0) {
      allocator->free_pages--;
    }
  }

  // 5. ページ 0 (物理アドレス 0x0) は NULL ポインタ誤判定を防ぐため常時「使用中 (1)」にして保護
  bitmap_set(allocator->bitmap, 0);
  if (allocator->free_pages > 0) {
    allocator->free_pages--;
  }
}

// ====================================================================================
// alloc_frame: 1 ページ (4KB) の物理メモリを割り当てる
// ====================================================================================
void *alloc_frame(BitmapFrameAllocator *allocator) {
  return alloc_frames(allocator, 1);
}

// ====================================================================================
// alloc_frames: 連続した複数ページ (4KB * count) の物理メモリを一括で割り当てる
// ====================================================================================
void *alloc_frames(BitmapFrameAllocator *allocator, unsigned long long count) {
  if (count == 0) {
    return 0;
  }

  unsigned long long consecutive_free = 0;
  unsigned long long start_page       = 0;

  // 連続した空きページ (0/FRAME_FREE) を走査
  for (unsigned long long i = 0; i < allocator->total_pages; i++) {
    if (bitmap_get(allocator->bitmap, i) == FRAME_FREE) {
      if (consecutive_free == 0) {
        start_page = i; // 開始ページインデックスを記録
      }
      consecutive_free++;

      // 必要ページ数分連続して見つかった場合
      if (consecutive_free == count) {
        for (unsigned long long j = 0; j < count; j++) {
          bitmap_set(allocator->bitmap, start_page + j);
        }
        allocator->free_pages -= count;
        return (void *)(start_page * PAGE_SIZE); // 先頭の物理アドレスを返却
      }
    } else {
      consecutive_free = 0; // 途切れたらカウントをリセット
    }
  }

  return 0; // 連続空き領域が見つからなかった場合
}

// ====================================================================================
// free_frame: 指定した物理ページ (4KB) を解放する
// ====================================================================================
void free_frame(BitmapFrameAllocator *allocator, void *frame_addr) {
  unsigned long long page_index = (unsigned long long)frame_addr / PAGE_SIZE;
  if (page_index < allocator->total_pages) {
    // 現在の状態が FRAME_USED (1) であることを確認してから解放
    if (bitmap_get(allocator->bitmap, page_index) == FRAME_USED) {
      bitmap_clear(allocator->bitmap, page_index);  // FRAME_FREE (0) に戻す
      allocator->free_pages++;
    }
  }
}
