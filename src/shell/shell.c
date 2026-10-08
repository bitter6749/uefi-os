#include "shell/shell.h"
#include "drivers/keyboard.h"
#include "drivers/console.h"

// 簡易的な文字列比較kannsuu 
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

static void shell_execute_command(KernelShell *shell) {
  console_puts(shell->con, "\n"); // 改行

  // 入力が空の場合はプロンプトを再表示して終了
  if (shell->line_len == 0) {
    shell_print_prompt(shell);
    return;
  }

  // --- コマンド判定ロジック ---
  if (k_strcmp(shell->line_buf, "help") == 0) {
    console_puts(shell->con, "Avaliable commands:\n");
    console_puts(shell->con, "  help  - Display this help message\n");
    console_puts(shell->con, "  clear - Clear the screen\n");
  } else if (k_strcmp(shell->line_buf, "clear") == 0) {
    // コンソソールクリア処理
    console_clear(shell->con);
  } else {
    console_puts(shell->con, "Command not found: ");
    console_puts(shell->con, shell->line_buf);
    console_puts(shell->con, "\n");
  }

  // バッファをリセットしてプロンプトを表示
  shell->line_len     = 0;
  shell->line_buf[0]  = '\0';
  shell_print_prompt(shell);
}

void shell_init(KernelShell *shell, Console *con) {
  shell->con = con;
  shell->line_len = 0;
  shell->line_buf[0] = '\0';

  console_puts(shell->con, "=== Grapchis Kernel Shell ===\n");
  shell_print_prompt(shell);
}

void shell_update(KernelShell *shell) {
  char c = keyboard_getchar();
  if (c == '\0') {
    return; // 入力なし
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

