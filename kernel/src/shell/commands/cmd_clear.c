#include "shell/command.h"
#include "drivers/console.h"

// ============================================================================
// cmd_clear: コンソール画面を消去する
// ============================================================================
DEFINE_COMMAND(clear) {
  (void)argc;
  (void)argv;
  console_clear(shell->con);
}
