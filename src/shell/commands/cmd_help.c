#include "shell/command.h"
#include "drivers/console.h"

// ============================================================================
// cmd_help: 利用可能なコマンド一覧を表示する
// ============================================================================
DEFINE_COMMAND(help) {
  (void)argc;
  (void)argv;
  console_puts(shell->con, "Availabe commands:\n");
  for (unsigned int i = 0; i < g_shell_commands_count; i++) {
    console_puts(shell->con, "  ");
    console_puts(shell->con, g_shell_commands[i].name);
    console_puts(shell->con, " - ");
    console_puts(shell->con, g_shell_commands[i].description);
    console_puts(shell->con, "\n");
  }
}
