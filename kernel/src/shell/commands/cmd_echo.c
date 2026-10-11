#include "shell/command.h"
#include "drivers/console.h"

// ============================================================================
// cmd_echo: 入力された文字列をそのまま表示する
// ============================================================================
DEFINE_COMMAND(echo) {
  for (int i = 1; i < argc; i++) {
    console_puts(shell->con, argv[i]);
    if (i < argc - 1) {
      console_puts(shell->con, " ");
    }
  }
  console_puts(shell->con, "\n");
}
