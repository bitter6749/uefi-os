#ifndef TYPE_H
#define TYPE_H

typedef unsigned long long EFI_STATUS;
#define EFI_SUCCESS 0

// テーブルヘッダ (署名やサイズ情報)
typedef struct {
  unsigned long long Signature;
  unsigned int       Revision;
  unsigned int       HeaderSize;
  unsigned int       CRC32;
  unsigned int       Reserved;
} EFI_TABLE_HEADER;

// UEFI Reset Type
typedef enum {
  EfiResetCold = 0,
  EfiResetWarm  = 1,
  EfiResetShutdown = 2,
  EfiResetPlatformSpecific = 3,
} EFI_RESET_TYPE;

// ResetSystem の関数ポインタ型
typedef void (*EFI_RESET_SYSTEM) (
  EFI_RESET_TYPE ResetType,
  EFI_STATUS ResetStatus,
  unsigned long long DataSize,
  void *ResetData
);

// Runtime Services 構造体の最小限の定義
typedef struct {
  EFI_TABLE_HEADER Header;
  void *GetTime;
  void *SetTime;
  void *GetWakeupTime;
  void *SetWakeupTime;
  void *SetVirtualAddressMap;
  void *ConvertPointer;
  void *GetVariable;
  void *GetNextVariableName;
  void *SetVariable;
  void *GetNextHighMonotonicCount;
  EFI_RESET_SYSTEM ResetSystem; // シャットダウン関数
} EFI_RUNTIME_SERVICES;

// =====================================================================
// メモリ領域の種別 (EFI_MEMORY_TYPE)
// =====================================================================
typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimesServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,  // 使用可能な空き物理メモリ領域
  EfiUnsableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedPortSpace,
  EfiPalCode,
  EfiPersistentMemory,
  EfiMaxMemoryType,
} EFI_MEMORY_TYPE;

// =====================================================================
// メモリ記述子構造体 (EFI_MEMORY_DESCRIPTOR)
// =====================================================================
// 各物理メモリ領域の先頭アドレス、ページ数、種別を保持する
typedef struct {
  unsigned int        Type;             // メモリ領域種別 (EFI_MEMORY_TYPE)
  unsigned int        Pad;              // 8バイトのアライメント用のパッディング
  unsigned long long  PhysicalStart;    // 領域の先頭物理アドレス
  unsigned long long  VirtualStart;     // 領域の先頭仮想アドレス
  unsigned long long  NumberOfPages;    // ページ数 (1ページ = 4KB = 4096バイト)
  unsigned long long  Attribute;        // メモリ属性フラグ
} EFI_MEMORY_DESCRIPTOR;

#endif
