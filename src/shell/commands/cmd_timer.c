#include "shell/command.h"
#include "drivers/console.h"
#include "arch/lapic.h"

// DEFINE_COMMAND(ticks) で void cmd_ticks(KernelShell *shell, int argc, char **argv) に展開される
DEFINE_COMMAND(ticks) {
  (void)argc;
  (void)argv;

  unsigned long long now_ticks = lapic_get_ticks();
  console_puts(shell->con, "Ticks: ");
  console_put_dec(shell->con, now_ticks);
  console_puts(shell->con, "\n");
}
