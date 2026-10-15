#include "shell/command.h"
#include "drivers/console.h"
#include "drivers/pci.h"

DEFINE_COMMAND(ahci) {
  (void)argc;
  (void)argv;

  PCIDevice dev;
  if (!pci_find_ahci(&dev)) {
    console_puts(shell->con, "AHCI controller not found.\n");
    return;
  }

  console_puts(shell->con, "=== AHCI Controller Detected ===\n");

  console_puts(shell->con, "  PCI Location : ");
  console_put_hex(shell->con, dev.bus, 2);
  console_puts(shell->con, ":");
  console_put_hex(shell->con, dev.device, 2);
  console_puts(shell->con, ".");
  console_put_dec(shell->con, dev.function);
  console_puts(shell->con, "\n");

  console_puts(shell->con, "  Vendor ID    : ");
  console_put_hex(shell->con, dev.vendor_id, 4);
  console_puts(shell->con, "\n");

  console_puts(shell->con, "  Device ID    : ");
  console_put_hex(shell->con, dev.device_id, 4);
  console_puts(shell->con, "\n");

  console_puts(shell->con, "  Class / Sub  : ");
  console_put_hex(shell->con, dev.class_code, 2);
  console_puts(shell->con, " / ");
  console_put_hex(shell->con, dev.subclass, 2);
  console_puts(shell->con, " (ProgIF: ");
  console_put_hex(shell->con, dev.prog_if, 2);
  console_puts(shell->con, ")\n");

  console_puts(shell->con, "  ABAR (BAR5)  : ");
  console_put_hex(shell->con, dev.bar5, 8);
  console_puts(shell->con, "\n");
}
