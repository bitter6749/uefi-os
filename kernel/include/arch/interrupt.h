#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "drivers/console.h"

// ====================================================================================
// x86_64 CPU 例外ベクター番号の定義
// ====================================================================================
#define VEC_DE  0       // #DE: Divide Error
#define VEC_DB  1       // #DB: Debug Exception
#define VEC_NMI 2       // NMI: Non-Maskable Interrupt
#define VEC_BP  3       // #BP: Breakpoint
#define VEC_OF  4       // #OF: Overflow
#define VEC_BR  5       // #BR: BOUND Range Exceeded
#define VEC_UD  6       // #UD: Invalid Opcode
#define VEC_NM  7       // #NM: Device Not Avaliable
#define VEC_DF  8       // #DF: Double Fault
#define VEC_TS  10      // #TS: Invalid TSS
#define VEC_NP  11      // #NP: Segment Not Present
#define VEC_SS  12      // #SS: Stack Fault
#define VEC_GP  13      // #GP: General Protection Fault
#define VEC_PF  14      // #PF: Page Fault

// デバッグ・ログ表示用の定数
#define HEX_DIGITS_64BIT  16

// 外部ハードウェア割り込み (IRQ) の開始ベクタ番号 (0~31 は CPU 例外)
#define IRQ_VECTOR_START  32

// ====================================================================================
// InterruptFrame: アセンブリ (interrupt.S) から渡されるスタックレイアウト構造体
// ====================================================================================
// PUSH_ALL および CPU の自動スタック退避順序に厳密に一致させる
// アセンブリから C ハンドラへの呼び出しは Windows x64 ABI (第1引数 = RCX) で渡す
typedef struct {
  // PUSH__ALL で保存された汎用レジスタ (15個)
  unsigned long long rax;
  unsigned long long rbx;
  unsigned long long rcx;
  unsigned long long rdx;
  unsigned long long rsi;
  unsigned long long rdi;
  unsigned long long rbp;
  unsigned long long r8;
  unsigned long long r9;
  unsigned long long r10;
  unsigned long long r11;
  unsigned long long r12;
  unsigned long long r13;
  unsigned long long r14;
  unsigned long long r15;

  // interrupt.s の各 ISR ラベルで PUSH された値
  unsigned long long vector;      // 割り込み番号 (0 ~ 255)
  unsigned long long error_code;  // エラーコード (なければダミー 0)

  // 割り込み発生時に CPU が自動的にスタックへ積んだ値
  unsigned long long rip;       // 割り込みが発生した命令アドレス
  unsigned long long cs;        // コードセグメント
  unsigned long long rflags;    // CPU フラグレジスタ
  unsigned long long rsp;       // 割り込み発生時のスタックポインタ
  unsigned long long ss;        // スタックセグメント
} __attribute__((packed)) InterruptFrame;

// ====================================================================================
// 外部公開用アセンブリ ISR (Interrupt Service Routine) シンボルの宣言
// ====================================================================================
void isr0(void);                // #DE: Divide Error
void isr14(void);               // #PF: Page Fault
void isr32(void);               // タイマー割り込み
void isr33(void);               // キーボード割り込み
void isr_stub_default(void);    // デフォルトハンドラ

// ====================================================================================
// exception_handler: アセンブリから呼び出される共通 C 例外ハンドラ
// ====================================================================================
// - 引数1 (frame): スタック上のレジスタ情報を保持する構造体へのポインタ
void exception_handler(InterruptFrame *frame);

// コンソール出力用のインスタンスの共有登録
void interrupt_set_console(Console *con);

#endif
