#include "paging.h"
#include "memory.h"

// ページテーブル用メモリ (4KB) を割り当てて 0 でクリアする内部ヘルパー関数
static PageTable *create_empty_table(BitmapFrameAllocator *allocator) {
  PageTable *table = (PageTable *)alloc_frame(allocator);
  if (!table) {
    return 0;
  }

  // 確保したテーブル内を全要素 0 (無効) でクリア
  for (int i = 0; i < PAGE_ENTRIES_PER_TABLE; i++) {
    table->entries[i] = 0;
  }

  return table;
}

// ====================================================================================
// setup_identity_mapping: 物理アドレスと仮想アドレスを 1 対 1 でマッピングする
// ====================================================================================
PageTable *setup_identity_mapping(BitmapFrameAllocator *allocator, unsigned long long max_phys_addr) {
  // 1. ルートとなる PML4 テーブルの作成
  PageTable *pml4 = create_empty_table(allocator);
  if (!pml4) {
    return 0;
  }

  // マッピングに必要な全ページ数を算出
  unsigned long long total_pages = (max_phys_addr + PAGE_SIZE - 1) / PAGE_SIZE;

  // 属性フラグ: Present (存在) | Writable (読み書き可)
  unsigned long long flags = PAGE_ENTRY_PRESENT | PAGE_ENTRY_WRITABLE;

  // 2. 物理アドレス 0 から max_phys_addr までを順に 4KB 単位で辿ってマッピング
  for (unsigned long long page_idx = 0; page_idx < total_pages; page_idx++) {
    unsigned long long phys_addr = page_idx * PAGE_SIZE;

    // 階層ごとのインデックス計算 (各9ビット)
    unsigned long long pml4_idx = (phys_addr >> 39) & 0x1FF;
    unsigned long long pdpt_idx = (phys_addr >> 30) & 0x1FF;
    unsigned long long pd_idx   = (phys_addr >> 21) & 0x1FF;
    unsigned long long pt_idx   = (phys_addr >> 12) & 0x1FF;

    // --- Level 4: PML4 -> PDPT ---
    PageTable *pdpt = 0;
    if (pml4->entries[pml4_idx] & PAGE_ENTRY_PRESENT) {
      pdpt = (PageTable *)(pml4->entries[pml4_idx] & ~0xFFFULL);
    } else {
      pdpt = create_empty_table(allocator);
      if (!pdpt) return 0;
      pml4->entries[pml4_idx] = (unsigned long long)pdpt | flags;
    }

    // --- Level 3: PDPT -> PD ---
    PageTable *pd = 0;
    if (pdpt->entries[pdpt_idx] & PAGE_ENTRY_PRESENT) {
      pd = (PageTable *)(pdpt->entries[pdpt_idx] & ~0xFFFULL);
    } else {
      pd = create_empty_table(allocator);
      if (!pd) return 0;
      pdpt->entries[pdpt_idx] = (unsigned long long)pd | flags;
    }

    // --- Level 2: PD -> PT ---
    PageTable *pt = 0;
    if (pd->entries[pd_idx] & PAGE_ENTRY_PRESENT) {
      pt = (PageTable *)(pd->entries[pd_idx] & ~0xFFFULL);
    } else {
      pt = create_empty_table(allocator);
      if (!pt) return 0;
      pd->entries[pd_idx] = (unsigned long long)pt | flags;
    }

    // --- Level 1: PT -> 最終物理アドレス ---
    pt->entries[pt_idx] = phys_addr | flags;
  }

  return pml4;
}

// ====================================================================================
// load_pml4: インラインアセンブリで CR3 レジスタを更新し、ページテーブルを切り替える
// ====================================================================================
void load_pml4(PageTable *pml4_addr) {
  __asm__ volatile(
      "mov %0, %%cr3"
      :
      : "r"(pml4_addr)
      : "memory"
    );
}




