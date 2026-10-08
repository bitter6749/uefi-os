#include "efi.h"
#include "drivers/graphics.h"
#include "drivers/console.h"
#include "memmap/heap.h"
#include "arch/idt.h"
#include "arch/interrupt.h"
#include "drivers/keyboard.h"
#include "arch/lapic.h"
#include "memmap/paging.h"
#include "arch/pic.h"
#include "arch/ioapic.h"
#include "shell/shell.h"

// メモリマップ格納用の静的バッファ (4096 * 4 バイト = 16KB)
static unsigned char memory_map_buffer[4096 * 4];

// ============================================================================
// get_memory_map: UEFI からメモリマップを取得する
// ============================================================================
// - 引数1 (system_table):  UEFI システムテーブルへのポインタ
// - 引数2 (map):           取得した情報を格納する MemoryMap 構造体へのポインタ
// - 戻り値:                EFI_STATUS (0 = EFI_SUCCESS)
EFI_STATUS get_memory_map(EFI_SYSTEM_TABLE *system_table, MemoryMap *map) {
  map->buffer_size  = sizeof(memory_map_buffer);
  map->buffer       = memory_map_buffer;
  map->map_size     = map->buffer_size;

  // GetMemoryMap の引数順序:
  // 1. MapSize (*MemoryMapSize)
  // 2. MemoryMap (*MemoryMap)
  // 3. MapKey (*MapKey)
  // 4. DescriptorSize (*DescriptorSize)
  // 5. DescriptorVersion (*DescriptorVersion)
  typedef EFI_STATUS (*GetMemoryMapType)(
    unsigned long long    *MemoryMapSize,
    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    unsigned long long    *MapKey,
    unsigned long long    *DescriptorSize,
    unsigned int          *DescriptorVersion
  );

  GetMemoryMapType get_map_func = (GetMemoryMapType)system_table->BootServices->GetMemoryMap;

  return get_map_func (
    &map->map_size,
    (EFI_MEMORY_DESCRIPTOR *)map->buffer,
    &map->map_key,
    &map->descriptor_size,
    &map->descriptor_version
  );
}

// ============================================================================
// print_memory_map: 取得したメモリマップ情報を画面に出力する
// ============================================================================
// - 引数1 (con):   出力先コンソール構造体へのポインタ
// - 引数2 (map):   メモリマップ構造体へのポインタ
void print_memory_map(Console *con, MemoryMap *map) {
  console_puts(con, "--- MEMORY MAP ---\n");

  unsigned long long total_conventional_bytes = 0;
  unsigned long long total_boot_services_byte = 0;

  unsigned long long offset = 0;

  while (offset < map->map_size) {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((unsigned long long)map->buffer + offset);
    unsigned long long bytes = desc->NumberOfPages * 4096;

    // 空きメモリ (EfiConventionalMemory = 7) のみを表示する例
    if (desc->Type == EfiConventionalMemory) {
      total_conventional_bytes += bytes;
   } else if (desc->Type == EfiBootServicesCode || desc->Type == EfiBootServicesData) {
     total_boot_services_byte += bytes;
   }
   offset += map->descriptor_size;
  }

  console_puts(con, "Current Free Memory  : ");
  console_put_dec(con, total_conventional_bytes / (1024 * 1024));
  console_puts(con, " MB\n");

  console_puts(con, "Reclaimable Memory   : ");
  console_put_dec(con, total_boot_services_byte / (1024 * 1024));
  console_puts(con, " MB\n");

  console_puts(con, "Total Avaliable   : ");
  console_put_dec(con, (total_conventional_bytes + total_boot_services_byte) / (1024 * 1024));
  console_puts(con, " MB\n");
}

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
  FrameBuffer fb = {
    .base = (unsigned int *)gop->Mode->FrameBufferBase,
    .width = gop->Mode->Info->HorizontalResolution, // 画面の横幅(px)
    .height = gop->Mode->Info->VerticalResolution,  // 画面の縦幅(px)
                                                    
    // --- PixelPerScanLine ---
    // ディスプレイの伝送効率やハードウェアの境界合わせのため、画面右端に「目に見えない余白」
    // が含まれることがある。そのため、1行進める計算には width ではなく ppsl を必ず使う。
    .ppsl = gop->Mode->Info->PixelPerScanLine
  };

  // 背景をネイビーブルーでクリア
  clear_screen(&fb, 0x001E3F);

  // 4. コンソールの初期化 (文字色: 白 0xFFFFFF, 背景色: ネイビー 0x001E3F)
  Console con;
  console_init(&con, &fb, 0xFFFFFF, 0x001E3F);

  // メモリマップの取得
  MemoryMap map;
  EFI_STATUS status_map = get_memory_map(system_table, &map);

  if (status_map == EFI_SUCCESS) {
    console_puts(&con, "Successfully fetched Memory Map!\n");
    print_memory_map(&con, &map);

    // --- 物理フレームアロケータの初期化 ---
    BitmapFrameAllocator allocator;
    frame_allocator_init(&allocator, &map);

    console_puts(&con, "Total Pages: ");
    console_put_dec(&con, allocator.total_pages);
    console_puts(&con, " Free Pages: ");
    console_put_dec(&con, allocator.free_pages);
    console_puts(&con, "\n");

    // --- ヒープアロケータの初期化 (16ページ = 64KB を確保) ---
    HeapAllocator heap;
    heap_init(&heap, &allocator, 16);
    console_puts(&con, "Heap initialized (64KB).\n");
    console_puts(&con, "Heap Block Size: ");
    console_put_dec(&con, heap.start_block->size);
    console_puts(&con, " bytes\n");

    // 1. メモリ確保テスト (kmalloc)
    int *arr = (int *)kmalloc(&heap, sizeof(int) * 5);
    if (arr) {
      console_puts(&con, "kmalloc array addr: ");
      console_put_hex(&con, (unsigned long long)arr, 16);
      console_puts(&con, "\n");

      // 値の読み書きテスト
      for (int i = 0; i < 5; i++) {
        arr[i] = (i + 1) * 10;
      }

      console_puts(&con, "arr[4] value: ");
      console_put_dec(&con, arr[4]);  // 50
      console_puts(&con, "\n");

      // 2. メモリ解放テスト (kfree)
      kfree(&heap, arr);
      console_puts(&con, "kfree success!\n");
    } else {
      console_puts(&con, "kmalloc failed\n");
    }



    console_puts(&con, "Total Pages: ");
    console_put_dec(&con, allocator.total_pages);
    console_puts(&con, " Free Pages: ");
    console_put_dec(&con, allocator.free_pages);
    console_puts(&con, "\n");

    // ページ割当テスト
    void *frame1 = alloc_frame(&allocator);
    console_puts(&con, "Allocated Frame 1: ");
    console_put_hex(&con, (unsigned long long)frame1, 16);
    console_puts(&con, "\n");

    // ページ解放テスト
    free_frame(&allocator, frame1);
    console_puts(&con, "Freed Frame 1. Free Pages: ");
    console_put_dec(&con, allocator.free_pages);
    console_puts(&con, "\n");

    // --- 64-bit ページテーブルの作成とCR3登録 ---
    unsigned long long max_phys_addr = get_max_physical_address(&map, &fb);

    // // LAPIC 領域 (0xFEE00000 付近) をマッピング範囲に含める
    if (max_phys_addr < 0xFEE00000ULL + 0x1000ULL) {
      max_phys_addr = 0xFEE00000ULL + 0x1000ULL;
    }

    // デバッグ出力で確認
    console_puts(&con, "\nFinal Max Addr: ");
    console_put_hex(&con, max_phys_addr, 16);
    console_puts(&con, "\nMapping Page Tables...\n");

    PageTable *pml4 = setup_identity_mapping(&allocator, max_phys_addr);

    if (pml4) {
      console_puts(&con, "PML4 Table Created at: ");
      console_put_hex(&con, (unsigned long long)pml4, 16);
      console_puts(&con, "\n");

      // CR3 レジスタを更新して OS 独自のページテーブルに切り替える
      load_pml4(pml4);
      console_puts(&con, "Successfully Loaded CR3! Page Table active.\n");
    } else {
      console_puts(&con, "Failed to setup Page Table.\n");
    }

    // cli で割り込みを一時停止
    __asm__ volatile("cli");
    pic_disable();

    // --- IDT (割り込み記述しテーブル) の初期化 --- 
    interrupt_set_console(&con);  // 例外ハンドラ用コンソール登録
    idt_init(&allocator);
    console_puts(&con, "IDT Initialized successfully!\n");

    // I/O APIC の初期化
    ioapic_init();

    // // ========================================================================
    // // 例外発生テスト (ゼロ除算例外 #DE: Vector 0)
    // // ========================================================================
    // console_puts(&con, "Testing Interrupt Handler (#DE: Divide Error)...\n");
    
    // __asm__ volatile(
    //     "xor %%edx, %%edx\n\t"
    //     "mov $10, %%eax\n\t"
    //     "mov $0, %%ecx\n\t"
    //     "div %%ecx"
    //     :
    //     :
    //     : "eax", "edx", "ecx"
    // );
    
    // console_puts(&con, "Continuing Kernel Excution...\n");

    // キーボードドライバの初期化
    keyboard_init();
    console_puts(&con, "Keyboard Ready. Type something.\n");
    // Local APIC タイマーの初期化と開始
    lapic_timer_init();
    console_puts(&con, "Local APIC Timer Started!\n");

    // 動作確認用ループ
    // unsigned long long last_tick = 0;

    KernelShell shell;
    shell_init(&shell, &con);
    while (1) {
      // unsigned long long current_tick = lapic_get_ticks();

      // // 100 ticks ごとにカウントを出力して動作確認
      // if (current_tick - last_tick >= 100) {
      //   console_puts(&con, "Tick: ");
      //   console_put_dec(&con, current_tick);
      //   console_puts(&con, "\n");
      //   last_tick = current_tick;
      // }

      // シェルのキー入力受け取りと描画処理
      shell_update(&shell);

      // 次の割り込みが入るまで CPU を休憩させて省電力化
      __asm__ volatile("hlt");
    }
  } else {
    console_puts(&con, "Failed to get Memory Map.\n");
  }

  // 5. 描画結果を表示し続けるために待機
  while (1) {
    __asm__ volatile("hlt");
  }

  return EFI_SUCCESS;
}
