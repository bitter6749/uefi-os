#include "idt.h"
#include "interrupt.h"
#include "lapic.h"
#include "keyboard.h"

// 256 個の IDT エントリ配列 (CPU が参照する割り込みテーブルの実体)
static IDTEntry *idt = 0;
// static IDTEntry idt[IDT_ENTRIES];

// IDTR レジスタにロードするための構造体変数
static IDTR idtr;

// ====================================================================================
// idt_set_gate: 特定の割り込みベクトルにハンドラを設定する
// ====================================================================================
void idt_set_gate(unsigned char vector, void *handler, unsigned char attribute) {
  if (!idt) return;
  unsigned long long handler_addr = (unsigned long long)handler;

  // 指定ベクトルのエントリを指すポインタを取得
  IDTEntry *entry = &idt[vector];

  // UEFI が設定している現在の CS (Code Segment) レジスタを取得してセット
  unsigned short cs;
  __asm__ volatile("mov %%cs, %0" : "=r"(cs));

  // 64-bit アドレスを 3 つの領域に分割してセット
  entry->offset_low        = (unsigned short)(handler_addr & ADDR_LOW_MASK);
  entry->segment_selector  = cs; // GDT のカーネルコードセグメント (Kernel Code Segment)
  entry->ist               = 0;    // IST (Interrupt Stack Table) は未使用 (0)
  entry->type_attributes   = attribute;
  entry->offset_mid        = (unsigned short)((handler_addr >> ADDR_MID_SHIFT) & ADDR_MID_MASK);
  entry->offset_high       = (unsigned int)((handler_addr >> ADDR_HIGH_SHIFT) & ADDR_HIGH_MASK);
  entry->reserved          = 0;
}

// ====================================================================================
// idt_init: IDT を初期化し、 lidt 命令で CPU レジスタに登録する
// ====================================================================================
void idt_init(BitmapFrameAllocator *allocator) {
  // 4KB (1ページ) の領域を確保 (256エントリ = 4096バイト)
  unsigned long long raw_addr = (unsigned long long)alloc_frame(allocator);

  if (!raw_addr) {
    while (1) { __asm__ volatile("hlt"); }
  }

  idt = (IDTEntry *)raw_addr;
  if (!idt) {
    while (1) { __asm__ volatile("hlt"); }
  }

  // 1. 全 256 エントリにデフォルトハンドラを登録
  for (int i = 0; i < IDT_ENTRIES; i++) {
    idt_set_gate(i, (void *)isr_stub_default, IDT_ATTR_INTERRUPT_GATE);
  }

  // 2. 個別対応する CPU 例外ハンドラを登録
  idt_set_gate(VEC_DE, (void *)isr0, IDT_ATTR_INTERRUPT_GATE);     // #DE: Divide Error
  idt_set_gate(VEC_PF, (void *)isr14, IDT_ATTR_INTERRUPT_GATE);   // #PF: Page Fault
  idt_set_gate(VEC_TIMER, (void *)isr32, IDT_ATTR_INTERRUPT_GATE);  // タイマー割り込み
  idt_set_gate(VEC_KEYBOARD, (void *)isr33, IDT_ATTR_INTERRUPT_GATE);  // キーボード割り込み

  // 3. IDTR 構造体のセットアップ
  idtr.limit    = (sizeof(IDTEntry) * IDT_ENTRIES) - 1; // バイト長 - 1
  idtr.base     = (unsigned long long)idt;

  // 3. インラインアセンブリで lidt 命令を実行し、 CPU に IDT をロード
  __asm__ volatile(
      "lidt %0"
      :
      : "m"(idtr)
    );
}















