#ifndef SHELL_H
#define SHELL_H

#include "drivers/console.h"

#define SHELL_MAX_LINE  128   // 1行あたりの最大入力文字数
#define SHELL_PROMPT    "> "  // プロンプト表示

typedef struct {
  Console *con;                   // 描画先コンソールへの参照
  char line_buf[SHELL_MAX_LINE];  // 現在入力中の文字列バッファ
  int line_len;                   // バッファ内の現在の文字数
} KernelShell;

// ============================================================================
// shell_init: シェルの初期化
// ============================================================================
// - 引数1 (shell): 
// - 引数2 (con): 
void shell_init(KernelShell *shell, Console *con);

// ================================================================================
// shell_update: メインループから毎フレーム呼び出す更新処理 (キー入力を監視・処理)
// ================================================================================
// - 引数1 (shell): 
void shell_update(KernelShell *shell);

#endif
