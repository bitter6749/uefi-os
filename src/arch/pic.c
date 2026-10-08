#include "arch/pic.h"
#include "arch/io.h"

// ====================================================================================
// pic_disable: 8259A PIC の全割り込みマスク
// ====================================================================================
void pic_disable(void) {
  outb(PIC_MASTER_DATA, PIC_MASK_ALL);
  outb(PIC_SLAVE_DATA, PIC_MASK_ALL);
}














