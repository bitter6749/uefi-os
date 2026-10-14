#include "memmap/paging.h"
#include "memmap/memory.h"

// ====================================================================================
// ページテーブル用メモリ (4KB) を割り当てて 0 でクリアする内部ヘルパー関数
// ====================================================================================
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

// =================j===================================================================
// get_or_create_next_table: 次の階層のページテーブルを取得、なければ新規作成する
// ====================================================================================
// - 引数1 (current_table):   現在の階層のテーブルポインタ
// - 引数2 (index):           対象のエントリインデックス (0~511)
// - 引数3 (allocator):       メモリ割り当て用フレームアロケータ
// - 引数4 (flags):           エントリに設定する属性フラグ
// - 戻り値:                  次階層のテーブルポインタ (失敗時は 0)
static PageTable *get_or_create_next_table(
    PageTable *current_table, 
    unsigned long long index,
    BitmapFrameAllocator *allocator,
    unsigned long long flags
) {
  if (current_table->entries[index] & PAGE_ENTRY_PRESENT) {
    // 既存のテーブルアドレスを抽出 (下位12bitのフラグをマスク)
    return (PageTable *)(current_table->entries[index] & PAGE_ADDRESS_MASK);
  }

  // 新規テーブルの作成とエントリへの登録
  PageTable *next_table = create_empty_table(allocator);
  if (!next_table) {
    return 0;
  }

  current_table->entries[index] = (unsigned long long)next_table | flags;
  return next_table;
}

// ====================================================================================
// setup_pat_wc: IA32_PAT MSR (0x277) の PA2 を Write-Combining (0x01) に設定する
// ====================================================================================
static void setup_pat_wc(void) {
  unsigned int msr = 0x277;
  unsigned int low, high;
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  unsigned long long pat = ((unsigned long long)high << 32) | low;

  // PA2 (bits 16-23) を 0x01 (WC: Write-Combining) に設定
  pat &= ~(0xFFULL << 16);
  pat |= (0x01ULL << 16);

  low = (unsigned int)pat;
  high = (unsigned int)(pat >> 32);
  __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

// ====================================================================================
// setup_identity_mapping: 物理アドレスと仮想アドレスを 1 対 1 でマッピングする
// ====================================================================================
PageTable *setup_identity_mapping(BitmapFrameAllocator *allocator, unsigned long long max_phys_addr, FrameBuffer *fb) {
  // 1. PAT の PA2 を Write-Combining に初期化
  setup_pat_wc();

  // VRAM のアドレス範囲 (バイト単位) を取得
  unsigned long long vram_start = 0;
  unsigned long long vram_end   = 0;
  if (fb && fb->vram) {
    vram_start = (unsigned long long)fb->vram;
    vram_end   = vram_start + (unsigned long long)fb->ppsl * fb->height * sizeof(unsigned int);
  }

  // ルートとなる PML4 テーブルの作成
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
    unsigned long long pml4_idx = (phys_addr >> PML4_SHIFT) & PAGE_INDEX_MASK;
    unsigned long long pdpt_idx = (phys_addr >> PDPT_SHIFT) & PAGE_INDEX_MASK;
    unsigned long long pd_idx   = (phys_addr >> PD_SHIFT) & PAGE_INDEX_MASK;
    unsigned long long pt_idx   = (phys_addr >> PT_SHIFT) & PAGE_INDEX_MASK;

    // --- Level 4: PML4 -> PDPT ---
    PageTable *pdpt = get_or_create_next_table(pml4, pml4_idx, allocator, flags);
    if (!pdpt) return 0;

    // --- Level 3: PDPT -> PD ---
    PageTable *pd = get_or_create_next_table(pdpt, pdpt_idx, allocator, flags);
    if (!pd) return 0;

    // --- Level 2: PD -> PT ---
    PageTable *pt = get_or_create_next_table(pd, pd_idx, allocator, flags);
    if (!pt) return 0;

    // --- Level 1: PT -> 最終物理アドレス ---
    unsigned long long page_flags = flags;
    // VRAM 領域であれば PCD(bit 4) を付与して PA2 (WC) に設定
    if (phys_addr >= vram_start && phys_addr < vram_end) {
      page_flags |= PAGE_ENTRY_PCD;
    }

    pt->entries[pt_idx] = phys_addr | page_flags;
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




