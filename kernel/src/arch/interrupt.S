.intel_syntax noprefix

// C 言語側で実装する例外ハンドラ関数の宣言
.extern exception_handler
.extern c_timer_handler
.extern c_keyboard_handler

// C 言語側 (idt.c など) から参照できるように関数シンボルを公開
.global isr0
.global isr14
.global isr32
.global isr33
.global isr_stub_default

// ====================================================================================
// 共通レジスタ保存・復元マクロ
// ====================================================================================
.macro PUSH_ALL
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rbp
  push rdi
  push rsi
  push rdx
  push rcx
  push rbx
  push rax
.endm

.macro POP_ALL
  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  pop rbp
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15
.endm

// ====================================================================================
// ISR (Interrupt Service Routine) ハンドラ定義
// ====================================================================================

// --- 割り込み 0 番: ゼロ除算例外 (#DE Divide Error) ---
// #DE はエラーコードを CPU が自動 push しないため、ダミー 0 を push してレイアウトを統一する
// InterruptFrame のメモリレイアウト: [rax..r15][vector][error_code][rip][cs][rflags][rsp][ss]
// スタックは push するたびに RSP が下がる (低アドレス方向) ため、
// 「後から push したもの」が RSP に近い = 構造体の先頭フィールドに対応する。
// よって error_code を先に push し、次に vector を push する。
isr0:
  push 0                // error_code フィールド用: ダミー 0
  push 0                // vector フィールド用: 割り込み番号 0
  jmp isr_common_stub

// --- 割り込み 14 番: ページフォルト例外 (#PF: Page Fault) ---
// #PF は CPU が自動的にエラーコードをスタックに push するため、vector のみ push する
isr14:
  push 14               // vector フィールド用: 割り込み番号 14
  jmp isr_common_stub

// --- 割り込み 32 番: Local APIC タイマー割り込み ---
isr32:
  PUSH_ALL              // レジスタ退避
  
  mov rcx, rsp          // x64 ABI 用 (第1引数)
  sub rsp, 40           // パッディング (32byte + 8byte alignment)
  call c_timer_handler  // C 言語のタイマーハンドラ呼び出し
  add rsp, 40

  POP_ALL               // レジスタ復元
  iretq                 // 割り込み復帰

// --- 割り込み 33 番: PS/2 キーボード割り込み ---
isr33:
  PUSH_ALL              // レジスタ退避
  
  mov rcx, rsp          // x64 ABI (第1引数)
  sub rsp, 40           // パッディング (32byte + 8byte alignment)
  call c_keyboard_handler // ハンドラ呼び出し
  add rsp, 40

  POP_ALL               // レジスタ復元
  iretq                 // 割り込み復帰

// --- デフォルトハンドラ (未登録の割り込み用) ---
isr_stub_default:
  push 0                // error_code フィールド用: ダミー 0
  push 255              // vector フィールド用: ダミー番号
  jmp isr_common_stub


// ====================================================================================
// isr_common_stub: 全例外共通のスタック構築・C ハンドラ呼び出し処理
// ====================================================================================
isr_common_stub:
  PUSH_ALL                // 15 個の汎用レジスタを退避 → InterruptFrame の rax..r15 フィールドに対応

  // Windows x64 ABI (x86_64-w64-mingw32-gcc) では第1引数は RCX で渡す。
  // System V AMD64 ABI (Linux/macOS) の RDI とは異なるため注意。
  mov rcx, rsp            // RCX = InterruptFrame* (スタック上の構造体の先頭)

  sub rsp, 40             // Windows x64 ABI のシャドウスペース (32byte) + 8byte アライメント調整
  call exception_handler  // C 言語の例外ハンドラを呼び出し
  add rsp, 40

  POP_ALL                 // レジスタ復元
  add rsp, 16             // vector + error_code の分 (8byte * 2) スタックを戻す
  iretq                   // 割り込みからの復帰
