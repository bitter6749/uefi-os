#ifndef SHELL_H
#define SHELL_H

#include "drivers/console.h"

#define SHELL_MAX_LINE  128   // 1行あたりの最大入力文字数
#define SHELL_PROMPT    "> "  // プロンプト表示

typedef struct KernelShell KernelShell;

// ============================================================================
// CommandHandler: 各コマンド処理関数の型定義 (関数ポインタ)
// ============================================================================
// - 引数1 (shell):   KernelShell 構造体へのポインタ
// - 引数2 (args):    コマンド引数文字列 (未指定時は空文字 "")
typedef void (*CommandHandler)(KernelShell *shell, const char *args);

// ============================================================================
// ShellCommand: コマンド名とハンドラ関数を対にする構造体
// ============================================================================
typedef struct {
  const char      *name;          // コマンド名 (例: "help")
  CommandHandler  handler;        // 実行する関数へのポインタ
  const char      *description;   // help 表示用の説明文
} ShellCommand;

typedef struct KernelShell{
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
