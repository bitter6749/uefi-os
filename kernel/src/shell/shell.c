#include "shell/shell.h"
#include "drivers/keyboard.h"
#include "drivers/console.h"
#include "shell/command.h"

// 簡易的な文字列複製関数
static void k_strcpy(char *dest, const char *src) {
  while ((*dest++ = *src++));
}

// 簡易的な文字列長取得関数
static int k_strlen(const char *s) {
  int len = 0;
  while (*s++) len++;
  return len;
}

// 簡易的な文字列比較関数 
static int k_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static void shell_print_prompt(KernelShell *shell) {
  console_puts(shell->con, SHELL_PROMPT);
}

static void shell_replace_line(KernelShell *shell, const char *new_str) {
  // 1. 現在画面に表示されている入力中の文字をバックスペースで消す
  while (shell->line_len > 0) {
    console_putc(shell->con, '\b');
    shell->line_len--;
  }

  // 2. 新しい文字列をバッファにセットして画面に描画する
  k_strcpy(shell->line_buf, new_str);
  shell->line_len = k_strlen(new_str);
  console_puts(shell->con, shell->line_buf);
}

static void shell_add_history(KernelShell *shell, const char *cmd) {
  if (cmd[0] == '\0') return; // 空行は保存しない

  // 直前の履歴と同じ場合は重複登録しない
  if (shell->history_count > 0) {
    int last_idx = (shell->history_head - 1 + HISTORY_MAX) % HISTORY_MAX;
    if (k_strcmp(shell->history[last_idx], cmd) == 0) {
      return;
    }
  }

  k_strcpy(shell->history[shell->history_head], cmd);
  shell->history_head = (shell->history_head + 1) % HISTORY_MAX;
  if (shell->history_count < HISTORY_MAX) {
    shell->history_count++;
  }
}

static void shell_execute_command(KernelShell *shell) {
  console_puts(shell->con, "\n"); // 改行

  // 入力が空の場合はプロンプトを再表示して終了
  if (shell->line_len == 0) {
    shell_print_prompt(shell);
    return;
  }

  // コマンドを実行前に履歴へ登録
  shell_add_history(shell, shell->line_buf);
  shell->history_index = -1; // 参照インデックスをリセット

  // 1. コマンドと引数を分離
  int argc = 0;
  char *argv[SHELL_MAX_ARGS];
  char *p = shell->line_buf;

  while (*p != '\0') {
    // 空白文字をスキップ
    while (*p == ' ') {
      *p = '\0';  // スペースをヌル終端に置換
      p++;
    }

    if (*p == '\0') {
      break;
    }

    // トークンの先頭ポインタを argv に保存
    if (argc < SHELL_MAX_ARGS) {
      argv[argc++] = p;
    }

    // 次の空白または末尾まで進める
    while (*p != '\0' && *p != ' ') {
      p++;
    }
  }

  // トークンが存在しない場合 (スペースのみの入力)
  if (argc == 0) {
    shell->line_len = 0;
    shell->line_buf[0] = '\0';
    shell_print_prompt(shell);
    return;
  }

  // 2. argv[0] (コマンド名) をテーブルから検索して実行
  int found = 0;
  for (unsigned int i = 0; i < g_shell_commands_count; i++) {
    if (k_strcmp(argv[0], g_shell_commands[i].name) == 0) {
      g_shell_commands[i].handler(shell, argc, argv);
      found = 1;
      break;
    }
  }

  // コマンドが見つからなかった時の処理
  if (!found) {
    console_puts(shell->con, "Command not found: ");
    console_puts(shell->con, argv[0]);
    console_puts(shell->con, "\n");
  }

  // バッファのリセット
  shell->line_len = 0;
  shell->line_buf[0] = '\0';
  shell_print_prompt(shell);
}

// ============================================================================
// shell_init: シェルの初期化
// ============================================================================
void shell_init(KernelShell *shell, Console *con) {
  shell->con  = con;
  shell->line_len = 0;
  shell->line_buf[0] = '\0';
  
  // 履歴管理変数の初期化
  shell->history_count  = 0;
  shell->history_head   = 0;
  shell->history_index  = -1;
  shell->temp_buf[0]    = '\0';

  console_puts(shell->con, "=== Grapchis Kernel Shell ===\n");
  shell_print_prompt(shell);
}

// ================================================================================
// shell_update: メインループから毎フレーム呼び出す更新処理 (キー入力を監視・処理)
// ================================================================================
void shell_update(KernelShell *shell) {
  char c = keyboard_getchar();
  if (c == '\0') {
    return; // 入力なし
  }

  if ((unsigned char)c == KEY_UP) {
    if (shell->history_count == 0) return;

    if (shell->history_index == -1) {
      // 初めて履歴を参照したとき : 現在入力中の文字列を temp_buf に退避
      k_strcpy(shell->temp_buf, shell->line_buf);
      shell->history_index = (shell->history_head - 1 + HISTORY_MAX) % HISTORY_MAX;
    } else {
      int oldest_idx = (shell->history_head - shell->history_count + HISTORY_MAX) % HISTORY_MAX;
      if (shell->history_index != oldest_idx) {
        shell->history_index = (shell->history_index - 1 + HISTORY_MAX) % HISTORY_MAX;
      }
    }
    shell_replace_line(shell, shell->history[shell->history_index]);
    return;
  }

  if ((unsigned char)c == KEY_DOWN) {
    if (shell->history_index == -1) return; // 履歴参照中でなければ無視

    int latest_idx = (shell->history_head - 1 + HISTORY_MAX) % HISTORY_MAX;
    if (shell->history_index == latest_idx) {
      // 最新の履歴から戻る場合 : 退避しておいた入力途中文字列を復元
      shell->history_index = -1;
      shell_replace_line(shell, shell->temp_buf);
    } else {
      shell->history_index = (shell->history_index + 1 + HISTORY_MAX) % HISTORY_MAX;
      shell_replace_line(shell, shell->history[shell->history_index]);
    }
    return;
  }

  // Enter キー押下時 -> コマンド実行
  if (c == '\n') {
    shell_execute_command(shell);
    return;
  }

  // BackSpace 押下時 -> バッファおよび画面から削る
  if (c == '\b') {
    if (shell->line_len > 0) {
      shell->line_len--;
      shell->line_buf[shell->line_len] = '\0';
      console_putc(shell->con, '\b');
    }
    return;
  }

  // 通常の文字入力 -> バッファ上限内で蓄積して描画
  if (shell->line_len < SHELL_MAX_LINE - 1) {
    shell->line_buf[shell->line_len++] = c;
    shell->line_buf[shell->line_len] = '\0';
    console_putc(shell->con, c);  // 画面に一文字描画
  }
}

