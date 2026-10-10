#include "arch/interrupt.h"
#include "arch/lapic.h"
#include "drivers/console.h"

static Console *global_console = 0;

// コンソール描画用インスタンスの保持
void interrupt_set_console(Console *con) {
  global_console = con;
}

// 例外番号に対応する名前文字列の取得
static const char *get_exception_name(unsigned long long vector) {
  switch (vector) {
    case VEC_DE:    return "#DE: Divide Error";
    case VEC_DB:    return "#DB: Debug Exception";
    case VEC_NMI:   return "NMI: Non-Maskable Interrupt";
    case VEC_BP:    return "#BP: Breakepoint";
    case VEC_OF:    return "#OF: OVerFlow";
    case VEC_BR:    return "#BR: BOUND Range Exceeded";
    case VEC_UD:    return "#UD: Invalid Opcode";
    case VEC_NM:    return "#NM: Device Not Avaliable";
    case VEC_DF:    return "#DF: Double Fault";
    case VEC_TS:    return "#TS: Invalid TSS";
    case VEC_NP:    return "#NP: Segment Not Present";
    case VEC_SS:    return "#SS: Stack Fault";
    case VEC_GP:    return "#GP: General Protection Fault";
    case VEC_PF:    return "#PF: Page Fault";
    default:        return "Unknown Exception";
  }
}

// ====================================================================================
// exception_handler: 例外発生時のカーネルパニック画面出力処理
// ====================================================================================
void exception_handler(InterruptFrame *frame) {
  // 32 番以降は外部 IRQ。未割当の割り込みは EOI を送信して安全に復帰させる
  if (frame->vector >= IRQ_VECTOR_START) {
    lapic_eoi();
    return;
  }

  if (!global_console) {
    // コンソールが未登録の場合は無限ループ停止
    while (1) { __asm__ volatile("hlt"); }
  }

  Console *con = global_console;

  // 背景色・文字色は変更せず赤文字等でメッセージ表示
  console_puts(con, "\n==============================================================\n");
  console_puts(con, "\n                  KERNEL PANIC (EXCEPTION)                    \n");
  console_puts(con, "\n==============================================================\n");

  console_puts(con, "Exception : ");
  console_puts(con, get_exception_name(frame->vector));
  console_puts(con, " (Vector: ");
  console_put_dec(con, frame->vector);
  console_puts(con, ")\n");

  console_puts(con, "Error Code: ");
  console_put_hex(con, frame->error_code, HEX_DIGITS_64BIT);
  console_puts(con, "\n");

  console_puts(con, "RIP      : ");
  console_put_hex(con, frame->rip, 16);
  console_puts(con, "   RSP: ");
  console_put_hex(con, frame->rsp, 16);
  console_puts(con, "\n");

  // Page Fault (#PF: 14) の場合は、CR2 レジスタ (アクセス失敗アドレス) を出力
  if (frame->vector == 14) {
    unsigned long long cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    console_puts(con, "Fault Address (CR2): ");
    console_put_hex(con, cr2, HEX_DIGITS_64BIT);
    console_puts(con, "\n");
  }

  console_puts(con, "==============================================================\n");
  console_puts(con, "System Halted.\n");

  // 例外発生後は安全のため CPU を無限停止
  while (1) {
    __asm__ volatile("hlt");
  }
}












