#include "keyboard.h"
#include "io.h"
#include "lapic.h"

static KeyBuffer g_key_buffer;

// ====================================================================================
// keyboard_init: キーボードドライバおよび内蔵バッファの初期化を行う
// ====================================================================================
void keyboard_init(void) {
  g_key_buffer.head = 0;
  g_key_buffer.tail = 0;
}

// バッファに 1 バイト増加 (割り込みハンドラ内から呼び出し)
static void keyboard_push_scancode(unsigned char scancode) {
  unsigned int next_head = (g_key_buffer.head + 1) % KBD_BUFFER_SIZE;
  // バッファフルでなければ格納
  if (next_head != g_key_buffer.tail) {
    g_key_buffer.buffer[g_key_buffer.head] = scancode;
    g_key_buffer.head = next_head;
  }
}

// ====================================================================================
// keyboard_pop_scancode: バッファからスキャンコードを 1 つ取り出す
// ====================================================================================
// - 戻り値: スキャンコード (どのキーを入力したか)
int keyboard_pop_scancode(void) {
  if (g_key_buffer.head == g_key_buffer.tail) {
    return KBD_SCANCODE_EMPTY; // バッファが空
  }

  unsigned char scancode = g_key_buffer.buffer[g_key_buffer.tail];
  g_key_buffer.tail = (g_key_buffer.tail + 1) % KBD_BUFFER_SIZE;
  return (int)scancode;
}

// ====================================================================================
// c_keyboard_handler: キーボード割り込みハンドラ (isr33 アセンブリスタブから呼び出し)
// ====================================================================================
void c_keyboard_handler(void) {
  // 1. ポート 0x64 からスキャンコードを読み取り
  while (inb(KBD_STATUS_PORT) & KBD_STATUS_OBF) {
    unsigned char scancode = inb(KBD_DATA_PORT);

    // 0xFA (ACK) や 0xAA (Self-Test Passed) などの応答コードは
    // リングバッファに入れずに無視 (無視せずとも処理をスキップ) する
    if (scancode != KBD_RESP_ACK && scancode != KBD_RESP_BAT_SUCCESS) {
      keyboard_push_scancode(scancode);
    }
  }

  // 3. 割り込み完了通知 (EOI) を Local APIC に送信
  lapic_eoi();
}















