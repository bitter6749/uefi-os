#include "memmap/heap.h"
#include "memmap/memory.h"

#define HEAP_HEADER_SIZE  sizeof(HeapHeader)

// 8バイトアライメント切り上げヘルパー関数
static inline unsigned long long align8(unsigned long long value) {
  return (value + ALIGN_MASK) & ~ALIGN_MASK;
}

// ====================================================================================
// heap_init: ヒープ領域を初期化する
// ====================================================================================
void heap_init(HeapAllocator *heap, BitmapFrameAllocator *allocator, unsigned long long initial_pages) {
  if (initial_pages == 0) {
    heap->start_block = 0;
    return;
  }

  // 1. 指定ページ数 (16ページ) の連続した物理フレームを一括取得する
  void *heap_start = alloc_frames(allocator, initial_pages);
  if (heap_start == 0) { // NULL/0 チェック
    heap->start_block = 0; // 連続領域の確保に失敗した場合
    return;
  }

  // 2. 先頭ブロックのヘッダー初期化
  unsigned long long total_bytes = initial_pages * PAGE_SIZE;
  unsigned long long header_size = align8(sizeof(HeapHeader));

  heap->start_block           = (HeapHeader *)heap_start;
  heap->start_block->size     = total_bytes - header_size;
  heap->start_block->is_free  = BLOCK_FREE;
  heap->start_block->next     = 0;
}

// ====================================================================================
// kmalloc: 指定サイズの動的メモリ領域を確保する (malloc 相当)
// ====================================================================================
void *kmalloc(HeapAllocator *heap, unsigned long long size) {
  if (size == 0 || heap->start_block == 0) {
    return 0;
  }

  unsigned long long req_size = align8(size);
  HeapHeader *curr = heap->start_block;

  // First-Fit 検索: 空きかつサイズが足りるブロックを探す
  while (curr) {
    if (curr->is_free == BLOCK_FREE && curr->size >= req_size) {
      // ブロック分割が可能か判定 (あまりが ヘッダーサイズ + 8バイト以上あれば分割)
      unsigned long long header_size = align8(sizeof(HeapHeader));
      if (curr->size >= req_size + HEAP_HEADER_SIZE + MIN_SPLIT_THRESHOLD) {
        HeapHeader *next_block  = (HeapHeader *)((unsigned long long)curr + HEAP_HEADER_SIZE + req_size);
        next_block->size        = curr->size - req_size - header_size;
        next_block->is_free     = BLOCK_FREE;
        next_block->next        = curr->next;

        curr->size              = req_size;
        curr->next              = next_block;
      }

      curr->is_free = BLOCK_USED;
      // ヘッダー直後のデータ領域のアドレスを返す
      return (void *)((unsigned long long)curr + header_size);
    }

    curr = curr->next;
  }

  return 0; // 確保失敗
}

// ====================================================================================
// kfree: 確保された動的メモリ領域を解放する (free 相当)
// ====================================================================================
void kfree(HeapAllocator *heap, void *ptr) {
  if (!ptr) {
    return;
  }

  // ポインタからヘッダー位置を計算
  HeapHeader *header = (HeapHeader *)((unsigned long long)ptr - HEAP_HEADER_SIZE);
  header->is_free = BLOCK_FREE;

  // 隣接する空きブロックの統合 (Coalescing)
  HeapHeader *curr = heap->start_block;
  while (curr && curr->next) {
    if (curr->is_free == BLOCK_FREE && curr->next->is_free == BLOCK_FREE) {
      curr->size += HEAP_HEADER_SIZE + curr->next->size;
      curr->next  = curr->next->next;
    } else {
      curr        = curr->next;
    }
  }
}
