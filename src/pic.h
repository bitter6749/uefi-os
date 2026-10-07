#ifndef PIC_H
#define PIC_H

// ====================================================================================
// 8259A PIC (Programmable Interrupt Controller) 定数定義
// ====================================================================================

// I/O ポートアドレス
#define PIC_MASTER_CMD      0x20    // Master PIC コマンドポート
#define PIC_MASTER_DATA     0x21    // Master PIC データポート
#define PIC_SLAVE_CMD       0xA0    // Slave PIC コマンドポート
#define PIC_SLAVE_DATA      0xA1    // Slave PIC データポート

// ICW (Initialization Command Words) 設定値
#define ICW1_INIT           0x10    // 初期化コマンドの開始時を示すビット
#define ICW1_ICW4           0x01    // ICW4 が送られることを示すビット
#define ICW4_8086           0x01    // 8086/88 モードに設定

// 割り込みベクターオフセット
#define PIC_MASTER_OFFSET   32      // Master PIC (IRQ0~7) -> Vector 32~39
#define PIC_SLAVE_OFFSET    40      // Slave PIC (IRQ8~15) -> Vector 40~47

// IRQ マスクビットパターン
#define PIC_MASK_ALL        0xFF    // 全IRQ をマスク (無効化)
#define PIC_MASK_IRQ1_ONLY  0xFD    // IRQ1 (PS/2キーボード) のみ許可 (1111 1101)

// ====================================================================================
// pic_remap: 8269A PIC の割り込みベクターを再配置し、必要な IRQ のみ許可する
// ====================================================================================
// - Master PIC (IRQ0~7) を Vector 32~39 へ割当 (IRQ1 -> Vector 33)
// - Slave PIC (IRQ8~15) を Vector 40~47 へ割当
// - IRQ1 (PS/2キーボード) 以外の不要な PIC 割り込みをマスク
void pic_remap(void);

// ====================================================================================
// pic_disable: 8259A PIC の全割り込みをマスク (無効化) する
// ====================================================================================
void pic_disable(void);

#endif // PIC_H
