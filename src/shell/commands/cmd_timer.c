#include "shell/command.h"
#include "drivers/console.h"

// DEFINE_COMMAND(ticks) で void cmd_ticks(KernelShell *shell, int argc, char **argv) に展開される
DEFINE_COMMAND(ticks) {
  (void)argc;
  (void)argv;
  console_puts(shell->con, "Ticks: 123456\n");
}
