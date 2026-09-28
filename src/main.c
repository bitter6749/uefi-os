// UEFI の戻り地の型 (64-bit 符号なし整数)
typedef unsigned long long EFI_STATUS;
#define EFI_SUCCESS 0

// UEFI のハンドラおよびポインタ型 (最初は void* で十分)
typedef void *EFI_HANDLE;

// UEFI ファームウェアから呼び出されるエントリポイント
EFI_STATUS efi_main(EFI_HANDLE image_handle, void *system_table) {
  (void)image_handle;
  (void)system_table;

  // 起動したことを確認するため、無限ループで CPU を停止 (待機) させる
  while (1) {
    __asm__ volatile("hlt");
  }

  return EFI_SUCCESS;
}
