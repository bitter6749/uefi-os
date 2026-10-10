#include "efi.h"
#include "shell/command.h"
#include "drivers/console.h"

extern EFI_RUNTIME_SERVICES *g_runtime_services;

DEFINE_COMMAND(exit) {
  (void)argc;
  (void)argv;

  console_puts(shell->con, "Shutting down system...\n");

  if (g_runtime_services && g_runtime_services->ResetSystem) {
    // EfiResetShutdown (2) を指定してシステムを電源オフ
    g_runtime_services->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
  }

  // シャットダウンに失敗した場合や復帰した場合のフォールバック
  __asm__ volatile("cli");
  while (1) {
    __asm__ volatile("hlt");
  }
}
