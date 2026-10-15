#include "shell/command.h"
#include "drivers/console.h"
#include "drivers/pci.h"

DEFINE_COMMAND(lspci) {
  (void)argc;
  (void)argv;

  int count = 0;
  for (unsigned int bus = 0; bus < 256; bus++) {
    for (unsigned int dev = 0; dev < 32; dev++) {
      for (unsigned int func = 0; func < 8; func++) {
        PCIDevice d;
        if (!pci_get_device(bus, dev, func, &d)) {
          // ファンクション0が無ければこのデバイス番号はスキップ
          if (func == 0) break;
          continue;
        }

        // 表示例: [00:00.0] 0x8086:0x1237 Bridge (0x06:0x00)
        console_puts(shell->con, "[");
        console_put_hex(shell->con, d.bus, 2);
        console_puts(shell->con, ":");
        console_put_hex(shell->con, d.device, 2);
        console_puts(shell->con, ".");
        console_put_dec(shell->con, d.function);
        console_puts(shell->con, "] ");

        console_put_hex(shell->con, d.vendor_id, 4);
        console_puts(shell->con, ":");
        console_put_hex(shell->con, d.device_id, 4);
        console_puts(shell->con, " ");

        console_puts(shell->con, pci_class_to_string(d.class_code));
        console_puts(shell->con, " (");
        console_put_hex(shell->con, d.class_code, 2);
        console_puts(shell->con, ":");
        console_put_hex(shell->con, d.subclass, 2);
        console_puts(shell->con, ")");

        // AHCI コントローラのときはマークを表示
        if (d.class_code == PCI_CLASS_STORAGE &&
            d.subclass == PCI_SUBCLASS_SATA &&
            d.prog_if == PCI_PROG_IF_AHCI) {
          console_puts(shell->con, " *AHCI*");
        }

        console_puts(shell->con, "\n");
        count++;

        // 単一機能デバイスなら func 1~7 をスキップ
        if (func == 0) {
          unsigned int reg0C = pci_read_config_32(bus, dev, func, 0x0C);
          unsigned char header_type = (reg0C >> 16) & 0xFF;

          if ((header_type & 0x80) == 0) break;
        }
      }
    }
  }

  console_puts(shell->con, "Total devices: ");
  console_put_dec(shell->con, count);
  console_puts(shell->con, "\n");
}
