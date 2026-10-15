#ifndef PCI_H
#define PCI_H

#define PCI_CONFIG_ADDRESS    0xCF8
#define PCI_CONFIG_DATA       0xCFC

// --- PCI クラスコードの定義 ---
#define PCI_CLASS_STORAGE     0x01
#define PCI_CLASS_NETWORK     0x02
#define PCI_CLASS_DISPLAY     0x03
#define PCI_CLASS_MULTIMEDIA  0x04
#define PCI_CLASS_MEMORY      0x05
#define PCI_CLASS_BRIDGE      0x06
#define PCI_CLASS_SERIAL_BUS  0x0C

// --- AHCI コントローラの識別コード ---
#define PCI_SUBCLASS_SATA     0x06
#define PCI_PROG_IF_AHCI      0x01

typedef struct {
  unsigned char   bus;
  unsigned char   device;
  unsigned char   function;
  unsigned short  vendor_id;
  unsigned short  device_id;
  unsigned char   class_code;
  unsigned char   subclass;
  unsigned char   prog_if;
  unsigned int    bar5; // AHCI Base Address (ABAR)
} PCIDevice;

// PCI 設定空間から 32bit 値を読みだす
unsigned int pci_read_config_32(
    unsigned char bus, 
    unsigned char dev,
    unsigned char func,
    unsigned char offset
);

// 指定した位置 (Bus, Device, Function) のデバイス情報を取得する
// デバイスが存在すれば 1、存在しなければ 0 を返す
int pci_get_device(
    unsigned char bus, 
    unsigned char dev, 
    unsigned char func, 
    PCIDevice *out_dev
); 

// クラスコードを人間が読める文字列に変換する (例: 0x01 -> "Storage")
const char *pci_class_to_string(unsigned char class_code);

// AHCI コントローラを探す
int pci_find_ahci(PCIDevice *out_dev);

#endif
