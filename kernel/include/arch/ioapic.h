#ifndef IOAPIC_H
#define IOAPIC_H

#define IOAPIC_BASE_ADDR    0xFEC00000ULL

// I/O APIC レジスタオフセット
#define IOAPIC_REG_ID       0x00
#define IOAPIC_REG_VER      0x01
#define IOAPIC_REG_REDTBL0  0x10 // Redirection Table IRQ0 用 (2 レジスタで 64bit)

// ====================================================================================
// ioapic_init: I/O APIC を初期化し、IRQ1 (キーボード) を Vector 33 へルーティングする
// ====================================================================================
void ioapic_init(void);

// ====================================================================================
// ioapic_set_irq: 特定の IRQ を指定ベクター番号および目的 APIC ID にマッピングする
// ====================================================================================
void ioapic_set_irq(unsigned char irq, unsigned char vector, unsigned char apic_id);

#endif
