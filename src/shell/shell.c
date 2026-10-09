#include "shell/shell.h"
#include "drivers/keyboard.h"
#include "drivers/console.h"

static void cmd_help(KernelShell *shell, int argc, char **argv);
static void cmd_clear(KernelShell *shell, int argc, char **argv);
static void cmd_echo(KernelShell *shell, int argc, char **argv);

// ============================================================================
// g_shell_commands: 登録済みシェルコマンドの定義テーブル
// ============================================================================
static const ShellCommand g_shell_commands[] = {
  {"help",  cmd_help,   "Display availabel commands"},
  {"clear", cmd_clear,  "Clear the console screen"},
  {"echo",  cmd_echo,   "Display a line of text"},
};

#define COMMAND_COUNT (sizeof(g_shell_commands) / sizeof(g_shell_commands[0]))

// 簡易的な文字列比較関数 
static int k_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// ============================================================================
// cmd_help: 利用可能なコマンド一覧を表示する
// ============================================================================
static void cmd_help(KernelShell *shell, int argc, char **argv) {
  (void)argc;
  (void)argv;
  console_puts(shell->con, "Availabe commands:\n");
  for (unsigned int i = 0; i < COMMAND_COUNT; i++) {
    console_puts(shell->con, "  ");
    console_puts(shell->con, g_shell_commands[i].name);
    console_puts(shell->con, " - ");
    console_puts(shell->con, g_shell_commands[i].description);
    console_puts(shell->con, "\n");
  }
}

// ============================================================================
// cmd_clear: コンソール画面を消去する
// ============================================================================
static void cmd_clear(KernelShell *shell, int argc, char **argv) {
  (void)argc;
  (void)argv;
  console_clear(shell->con);
}

// ============================================================================
// cmd_echo: 入力された文字列をそのまま表示する
// ============================================================================
static void cmd_echo(KernelShell *shell, int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    console_puts(shell->con, argv[i]);
    if (i < argc - 1) {
      console_puts(shell->con, " ");
    }
  }
  console_puts(shell->con, "\n");
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
  for (unsigned int i = 0; i < COMMAND_COUNT; i++) {
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

