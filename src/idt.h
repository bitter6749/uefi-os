#ifndef IDT_H
#define IDT_H

#include "memory.h"

// ============================================================================
// IDT (Interrupt Descriptor Table) の概要
// ============================================================================
// IDT は CPU に対して「どの割り込み番号 (0~255) が発生した時に、どの関数 (ハンドラ)
// を実行するか」を指示する割り込み記述子テーブルです。
//
// [割り込み発生 (例: #PF 例外 14番)]
//        │
//        ▼
// [ CPU 内蔵の IDTR レジスタ ] ───▶ [ IDT (Interrupt Descriptor Table) ]
//                                  │
//                                  ├─ 00: #DE (Division Error)
//                                  ├─ 13: #GP (General Protection)
//                                  ├─ 14: #PF (Page Fault)
//                                  └─ 32~: HW Interrupts (Timer / Keyboard ...)
//
// x86_64 では 1 つのエントリが 16 バイト (128 ビット) で構成され、
// ハンドラの先頭アドレス (64-bit) やアクセス権限 (DPL) などを保持します。

// x86_64 の IDT ベクトル総数 (0~255)
#define IDT_ENTRIES 256

// 属性フラグ定数
#define IDT_ATTR_INTERRUPT_GATE 0x8E    // Preset(1) | DPL=0(Kernel) | Type=0xE(Interrupt Gate)
#define IDT_ATTR_TRAP_GATE      0x8F    // Preset(1) | DPL=0(Kernel) | Type=0xF(Trap Gate)
#define IDT_ATTR_USER_GATE      0xEE    // Preset(1) | DPL=3(User)   | Type=0xE(Interrupt Gate)

// アドレス分割操作定数
#define ADDR_LOW_MASK     0xFFFFULL
#define ADDR_MID_MASK     0xFFFFULL
#define ADDR_HIGH_MASK    0xFFFFFFFFULL
#define ADDR_MID_SHIFT    16
#define ADDR_HIGH_SHIFT   32

// ====================================================================================
// IDTEntry: x86_64 IDT エントリ構造体 (16 バイト / 128 ビット)
// ====================================================================================
// 64-bit モードではハンドラのアドレス (64bit) を 3 つに分割して格納します
typedef struct {
  unsigned short  offset_low;         // アドレス bit 0..15
  unsigned short  segment_selector;   // コードセグメントセレクタ (GDT)
  unsigned char   ist;                // Interrupt Stack Table (通常は 0)
  unsigned char   type_attributes;    // ゲート種別・アクセス制限フラグ
  unsigned short  offset_mid;         // アドレス bit 16..31
  unsigned int    offset_high;        // アドレス bit 32..63
  unsigned int    reserved;           // 予約領域 (常に 0)
} __attribute__((packed)) IDTEntry;   // 構造体のメンバ間にパッディングをいれないと宣言

// ====================================================================================
// IDTR: IDTR レジスタ設定用構造体 (10バイト)
// ====================================================================================
// lidt 命令に渡すための構造体
typedef struct {
  unsigned short      limit;        // テーブルのサイズ (バイト数 - 1)
  unsigned long long  base;         // IDT 配列の先頭物理/仮想アドレス
} __attribute__((packed)) IDTR;     // 構造体のメンバ間にパッディングをいれないと宣言

// ====================================================================================
// idt_init: IDT の初期化 (256 個のテーブル構築と lidt の実行)
// ====================================================================================
void idt_init(BitmapFrameAllocator *allocator);

// ====================================================================================
// idt_set_gate: 特定の割り込みベクトルにハンドラを設定する関数
// ====================================================================================
// - 引数1 (vector):      割り込み番号 (0 ~ 255)
// - 引数2 (handler):     割り込み発生時に実行するハンドラ関数のアドレス
// - 引数3 (attribute):   属性フラグ (例: IDT_ATTR_INTERRUPT_GATE)
void idt_set_gate(unsigned char vector, void *handler, unsigned char attribute);

#endif 
