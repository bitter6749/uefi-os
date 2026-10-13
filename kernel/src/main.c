#include "bootinfo.h"
#include "memory.h"
#include "type.h"
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

// UEFI runtime service 群へのポインタ
void *g_runtime_services = 0;

// ============================================================================
// print_memory_map: 取得したメモリマップ情報を画面に出力する
// ============================================================================
// - 引数1 (con):   出力先コンソール構造体へのポインタ
// - 引数2 (map):   メモリマップ構造体へのポインタ
void print_memory_map(Console *con, BootMemoryMap *map) {
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
// kernel_main: カーネルのエントリーポイント
// ============================================================================
// - 引数1 (image_handle): このOSバイナリ自身を指すファームウェア管理の識別子
void kernel_main(BootInfo *boot_info) {
  g_runtime_services = boot_info->runtime_services;

  // --------------------------------------------------------------------------
  // 画面 (フレームバッファ / VRAM) のハードウェア情報を取得
  // --------------------------------------------------------------------------
  // FrameBufferBase: ディスプレイに直結しているビデオメモリ(VRAM)の先頭物理アドレス。
  // 1ピクセルは 32bit (4バイト: 通常は 0x00RRGGBB) なので unsigned int* として扱う。
  FrameBuffer fb = {
    .vram = (unsigned int *)boot_info->fb.base,
    .width = boot_info->fb.width, // 画面の横幅(px)
    .height = boot_info->fb.height,  // 画面の縦幅(px)
    // --- PixelPerScanLine ---
    // ディスプレイの伝送効率やハードウェアの境界合わせのため、画面右端に「目に見えない余白」
    // が含まれることがある。そのため、1行進める計算には width ではなく ppsl を必ず使う。
    .ppsl = boot_info->fb.ppsl,
    .back_buffer = 0
  };

  unsigned long long fb_bytes = (unsigned long long)fb.ppsl * fb.height * sizeof(unsigned int);
  unsigned long long needed_pages = (fb_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

  // 4. コンソールの初期化 (文字色: 白 0xFFFFFF, 背景色: ネイビー 0x001E3F)
  Console con;
  console_init(&con, &fb, 0xFFFFFF, 0x001E3F);

  BootMemoryMap map = {
    .buffer_size        = boot_info->map.buffer_size,
    .buffer             = boot_info->map.buffer,
    .map_size           = boot_info->map.map_size,
    .map_key            = boot_info->map.map_key,
    .descriptor_size    = boot_info->map.descriptor_size,
    .descriptor_version = boot_info->map.descriptor_version
  };

  // --- 物理フレームアロケータの初期化 ---
  BitmapFrameAllocator allocator;
  frame_allocator_init(&allocator, &map);

  fb.back_buffer = (unsigned int *)alloc_frames(&allocator, needed_pages);
  // 背景をネイビーブルーでクリア
  clear_screen(&fb, 0x001E3F);
  console_puts(&con, "Kernel Started successfully!\n");
  console_puts(&con, "back_buffer: ");
  console_put_hex(&con, (unsigned long long)fb.back_buffer, 16);
  console_puts(&con, "\n");

  print_memory_map(&con, &map);

  console_puts(&con, "Total Pages: ");
  console_put_dec(&con, allocator.total_pages);
  console_puts(&con, " Free Pages: ");
  console_put_dec(&con, allocator.free_pages);
  console_puts(&con, "\n");

  if (fb.back_buffer) {
    console_puts(&con, "Back Buffer allocated successfully!\n");
  } else {
    console_puts(&con, "Failed to allocate Back Buffer.\n");
  }

  // --- ヒープアロケータの初期化 (16ページ = 64KB を確保) ---
  HeapAllocator heap;
  heap_init(&heap, &allocator, 16); 
  console_puts(&con, "Heap initialized (64KB).\n");
  console_puts(&con, "Heap Block Size: ");
  console_put_dec(&con, heap.start_block->size);
  console_puts(&con, " bytes\n");

  // --- 64-bit ページテーブルの作成とCR3登録 ---
  unsigned long long max_phys_addr = get_max_physical_address(&map, &fb);

  // // LAPIC 領域 (0xFEE00000 付近) をマッピング範囲に含める
  if (max_phys_addr < 0xFEE00000ULL + 0x1000ULL) {
    max_phys_addr = 0xFEE00000ULL + 0x1000ULL;
  }
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

  // 描画結果を表示し続けるために待機
  while (1) {
    __asm__ volatile("hlt");
  }
}
