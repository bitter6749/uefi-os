#include "efi.h"
#include "bootinfo.h"

static unsigned char memory_map_buffer[4096 * 4];

// カーネルの関数の型定義
typedef void (*KernelEntryType)(BootInfo *);

// ============================================================================
// efi_main: UEFI アプリケーションのエントリポイント
// ============================================================================
// - 引数1 (image_handle): このOSバイナリ自身を指すファームウェア管理の識別子
// - 引数2 (system_table): UEFIファームウェアが用意した各種機能・テーブルへのポインタ
// - 戻り値: EFI_STATUS (0 = EFI_SUCCESS)
EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
  (void)image_handle; // 未使用引数の警告を防ぐ

  // --------------------------------------------------------------------------
  // 1. GOP (Graphics Output Protocol) の取得準備
  // --------------------------------------------------------------------------
  // UFEI は機能 (プロトコル) を名前ではなく 「128bit の固有ID (GUID)」 で識別する。
  EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;

  // --------------------------------------------------------------------------
  // 2. ファームウェアに GOP インターフェースを要求する
  // --------------------------------------------------------------------------
  // LocateProtocol: ファームウェアに対して「画面描画機能(GOP)のポインタをくれ」と問い合わせる。
  // 第2引数は通常 0 (NULL)。第3引数のポインタ変数 (&gop) に取得結果のアドレスが入る。
  EFI_STATUS status = system_table->BootServices->LocateProtocol(&gop_guid, 0, (void **)&gop);

  // 取得に失敗した (GOPが未対応、またはGUIDが間違っている) 場合は停止
  if (status != EFI_SUCCESS || !gop) {
    // GOP が見つからなかった場合は停止
    while (1) {
      __asm__ volatile("hlt");
    }
  }

  BootInfo boot_info;
  boot_info.fb.base = (unsigned int *)gop->Mode->FrameBufferBase;
  boot_info.fb.width = gop->Mode->Info->HorizontalResolution;
  boot_info.fb.height = gop->Mode->Info->VerticalResolution;
  boot_info.fb.ppsl = gop->Mode->Info->PixelPerScanLine;
  boot_info.runtime_services = system_table->RuntimeServices;

   // メモリマップの取得
  boot_info.map.buffer_size = sizeof(memory_map_buffer);
  boot_info.map.buffer      = memory_map_buffer;
  boot_info.map.map_size    = boot_info.map.buffer_size;

  typedef EFI_STATUS (*GetMemoryMapType)(
    unsigned long long      *MemoryMapSize, 
    EFI_MEMORY_DESCRIPTOR   *MemoryMap,
    unsigned long long      *MapKey,
    unsigned long long      *DescriptorSize,
    unsigned int            *DescriptorVersion
  );
  GetMemoryMapType get_map_func=  (GetMemoryMapType)system_table->BootServices->GetMemoryMap;

  get_map_func(
      &boot_info.map.map_size,
      (EFI_MEMORY_DESCRIPTOR *)boot_info.map.buffer,
      &boot_info.map.map_key,
      &boot_info.map.descriptor_size,
      &boot_info.map.descriptor_version
  );

  extern void kernel_main(BootInfo *);
  kernel_main(&boot_info);

  return EFI_SUCCESS;
}
