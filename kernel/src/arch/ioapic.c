#include "arch/ioapic.h"

// I/O APIC レジスタを書き換えよう MMIO アドレス構造体
typedef struct {
  volatile unsigned int reg_select; // オフセット 0x00: 読み書きしたいレジスタ番号を指定
  unsigned int reserved[3];         // 16 バイトアライメント
  volatile unsigned int window;     // オフセット 0x10: 実際のデータ読み書き窓口
} IOAPICRegister;

static IOAPICRegister *ioapic = (IOAPICRegister *)IOAPIC_BASE_ADDR;

// レジスタの読み出し
static unsigned int ioapic_read(unsigned char reg) {
  ioapic->reg_select = reg;
  return ioapic->window;
}

// レジスタへの書き込み
static void ioapic_write(unsigned char reg, unsigned int value) {
  ioapic->reg_select = reg;
  ioapic->window = value;
}

void ioapic_set_irq(unsigned char irq, unsigned char vector, unsigned char apic_id) {
  // 各 IRQ は 64-bit (32-bit レジスタ 2個) のRedirection Entry を持つ
  // 入力レジスタ番号: 0x10 + (irq * 2)
  unsigned char reg_low   = IOAPIC_REG_REDTBL0 + (irq * 2);
  unsigned char reg_high  = reg_low + 1;

  // 下位 32bit: Vector 番号、Delivery Mode (000: Fixed)、Physical Mode (0)、Mask OFF (0: 有効化)
  unsigned int low = vector;

  // 上位 32bit: Destination APIC ID (Bit 24-31)
  unsigned int high = ((unsigned int)apic_id) << 24;

  ioapic_write(reg_low, low);
  ioapic_write(reg_high, high);
}

void ioapic_init(void) {
  // IRQ1 (PS/2 キーボード) を Vector 33 へマッピング
  ioapic_set_irq(1, 33, 0);
}

















