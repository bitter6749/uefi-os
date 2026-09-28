#include "efi.h"

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

  // --------------------------------------------------------------------------
  // 3. 画面 (フレームバッファ / VRAM) のハードウェア情報を取得
  // --------------------------------------------------------------------------
  // FrameBufferBase: ディスプレイに直結しているビデオメモリ(VRAM)の先頭物理アドレス。
  // 1ピクセルは 32bit (4バイト: 通常は 0x00RRGGBB) なので unsigned int* として扱う。
  unsigned int *fb = (unsigned int *)gop->Mode->FrameBufferBase;
  unsigned int width = gop->Mode->Info->HorizontalResolution; // 画面の横幅(px)
  unsigned int height = gop->Mode->Info->VerticalResolution;  // 画面の縦幅(px)

  // --- PixelPerScanLine ---
  // ディスプレイの伝送効率やハードウェアの境界合わせのため、画面右端に「目に見えない余白」
  // が含まれることがある。そのため、1行進める計算には width ではなく ppsl を必ず使う。
  unsigned int ppsl = gop->Mode->Info->PixelPerScanLine;

  // 4. 画面全体を青色 (ネイビーブルー: 0x001E3F) で塗りつぶす
  for (unsigned int y = 0; y < height; y++) {
    for (unsigned int x = 0; x < width; x++) {
      fb[y * ppsl + x] = 0x001E3F;
    }
  }

  // 5. 描画結果を表示し続けるために待機
  while (1) {
    __asm__ volatile("hlt");
  }

  return EFI_SUCCESS;
}
