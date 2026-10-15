#include "drivers/pci.h"
#include "arch/io.h" 

// ====================================================================================
// pci_read_config_32: PCI 設定空間から 32bit 値を読み出す
// ====================================================================================
unsigned int pci_read_config_32(
    unsigned char bus, 
    unsigned char dev, 
    unsigned char func, 
    unsigned char offset
) {
  // アドレス形式: 
  // Bit31: 有効フラグ(1), 
  // Bit23-16:Bus, 
  // Bit15-11:Device, 
  // Bit10-8:Function,
  // Bit7-2:Offset
  unsigned int address = (unsigned int)(
      (bus << 16) | 
      (dev << 11) | 
      (func << 8) | 
      (offset & 0xFC) | 
      ((unsigned int)0x80000000)
  );
  outl(PCI_CONFIG_ADDRESS, address);
  return inl(PCI_CONFIG_DATA);
}

// ====================================================================================
// pci_class_to_string: クラスコードを文字列に変換する
// ====================================================================================
const char *pci_class_to_string(unsigned char class_code) {
  switch (class_code) {
    case PCI_CLASS_STORAGE:     return "Storage";
    case PCI_CLASS_NETWORK:     return "Network";
    case PCI_CLASS_DISPLAY:     return "Display";
    case PCI_CLASS_MULTIMEDIA:  return "Multimedia";
    case PCI_CLASS_MEMORY:      return "Memory";
    case PCI_CLASS_BRIDGE:      return "Bridge";
    case PCI_CLASS_SERIAL_BUS:  return "SerialBus";
    default:                    return "Unknown";
  }
}

// ====================================================================================
// pci_get_device: 指定位置の PCI デバイス情報を取得する
// ====================================================================================
int pci_get_device(
    unsigned char bus, 
    unsigned char dev, 
    unsigned char func, 
    PCIDevice *out_dev
) {
  unsigned int reg0 = pci_read_config_32(bus, dev, func, 0x00);
  unsigned short vendor_id = reg0 & 0xFFFF;

  // ベンダーID が 0xFFFF または 0x0000 の場合はデバイスが存在しない
  if (vendor_id == 0xFFFF || vendor_id == 0x0000) {
    return 0;
  }

  unsigned int reg08 = pci_read_config_32(bus, dev, func, 0x08);

  out_dev->bus        = bus;
  out_dev->device     = dev;
  out_dev->function   = func;
  out_dev->vendor_id  = vendor_id;
  out_dev->device_id  = (reg0   >> 16) & 0xFFFF;
  out_dev->class_code = (reg08  >> 24) & 0xFF;
  out_dev->subclass   = (reg08  >> 16) & 0xFF;
  out_dev->prog_if    = (reg08  >> 8) & 0xFF;
  out_dev->bar5       = pci_read_config_32(bus, dev, func, 0x24);

  return 1;
}

// ====================================================================================
// pci_find_ahci: AHCI コントローラーを探す
// ====================================================================================
int pci_find_ahci(PCIDevice *out_dev) {
  for (unsigned int bus = 0; bus < 256; bus++) {
    for (unsigned int dev = 0; dev < 32; dev++) {
      for (unsigned int func = 0; func < 8; func++) {
        PCIDevice pci_dev;
        if (!pci_get_device(bus, dev, func, &pci_dev)) {
          // ファンクション0が存在しなければ、このデバイス番号には何も乗っていないので次へ
          if (func == 0) break;
          continue;
        }

        // AHCI コントローラ (Storage / SATA / AHCI) か判定
        if ((pci_dev.class_code == PCI_CLASS_STORAGE) && 
            (pci_dev.subclass == PCI_SUBCLASS_SATA) &&
            (pci_dev.prog_if == PCI_PROG_IF_AHCI)
           ){
          *out_dev = pci_dev;
          return 1;
        }

        // マルチファンクションデバイスでなければ func 1 ~ 7 のスキャンをスキップ
        if (func == 0) {
          unsigned int reg0C = pci_read_config_32(bus, dev, func, 0x0C);
          unsigned char header_type = (reg0C >> 16) & 0xFF;
          if ((header_type & 0x80) == 0) break;
        }
      }
    }
  }
  return 0;
}




