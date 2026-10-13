#ifndef SHELL_H
#define SHELL_H

#include "drivers/console.h"

#define SHELL_MAX_LINE  128   // 1行あたりの最大入力文字数
#define SHELL_MAX_ARGS  8     // 最大引数トークン数 (argc の上限)
#define SHELL_PROMPT    "> "  // プロンプト表示

#define HISTORY_MAX     10    // 保持する過去コマンドの最大数
#define COMMAND_MAX_LEN 256   // コマンドの最大文字数

typedef struct KernelShell KernelShell;

// ============================================================================
// CommandHandler: 各コマンド処理関数の型定義 (関数ポインタ)
// ============================================================================
// - 引数1 (shell):   KernelShell 構造体へのポインタ
// - 引数2 (argc):    トークン (コマンド+引数) の総数
// - 引数3 (argv):    トークン文字列配列へのポンタ
typedef void (*CommandHandler)(KernelShell *shell, int argc, char **argv);

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

  // --- コマンド履歴用 ---
  char    history[HISTORY_MAX][COMMAND_MAX_LEN];  // 履歴保存用配列
  int     history_count;                          // 保存されている履歴のインデックス (HISTORY_MAX)
  int     history_head;                           // 次に書き込む履歴のインデックス
  int     history_index;                          // 現在参照している履歴の位置 (-1 は新規入力中)
  char    temp_buf[COMMAND_MAX_LEN];              // 履歴を参照する直前の入力途中の文字を一時退避するバッファ
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
