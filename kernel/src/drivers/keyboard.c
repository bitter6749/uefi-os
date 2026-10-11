#include "drivers/keyboard.h"
#include "arch/io.h"
#include "arch/lapic.h"

static KeyBuffer g_key_buffer;
static volatile int g_shift_pressed = SHIFT_RELLEASED; // shift キー押下状態フラグ

// --- Scan Code Set 1 -> ASCII 変換テーブル ---
// 通常時 (0x00 ~ 0x39)
static const char scancode_table_normal[SCANCODE_TABLE_MAX] = {
  0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', 
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
  0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',    0, ' '
};

// Shift 押下時
static const char scancode_table_shift[SCANCODE_TABLE_MAX] = {
  0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
  0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
  0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
  '*',    0, ' '
};

// ====================================================================================
// keyboard_init: キーボードドライバおよび内蔵バッファの初期化を行う
// ====================================================================================
void keyboard_init(void) {
  g_key_buffer.head = 0;
  g_key_buffer.tail = 0;
  g_shift_pressed   = SHIFT_RELLEASED;
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

    // Shift キーの状態更新 (Make/Break)
    if (scancode == SCANCODE_LSHIFT_MAKE || scancode == SCANCODE_RSHIFT_MAKE) {
      g_shift_pressed = SHIFT_PRESSED;
    } else if (scancode == SCANCODE_LSHIFT_BREAK || scancode == SCANCODE_RSHIFT_BREAK) {
      g_shift_pressed = SHIFT_RELLEASED;
    }

    // 0xFA (ACK) や 0xAA (Self-Test Passed) などの応答コードは
    // リングバッファに入れずに無視 (無視せずとも処理をスキップ) する
    if (scancode != KBD_RESP_ACK && scancode != KBD_RESP_BAT_SUCCESS) {
      keyboard_push_scancode(scancode);
    }
  }

  // 3. 割り込み完了通知 (EOI) を Local APIC に送信
  lapic_eoi();
}

// ====================================================================================
// scancode_to_ascii: スキャンコードを ASCII 文字へ変換する
// ====================================================================================
char scancode_to_ascii(unsigned char scancode) {
  // Break コード (キーを離したとき) は無視する (Bit 7 が 1)
  if ((scancode & SCANCODE_BREAK_BIT) || scancode == SCANCODE_PREFIX_E0 ) {
    return 0;
  }

  // テーブルの範囲外チェック
  if (scancode >= SCANCODE_TABLE_MAX) {
    return 0;
  }

  // Shift 状態に応じて文字を取得
  if (g_shift_pressed) {
    return scancode_table_shift[scancode];
  } else {
    return scancode_table_normal[scancode];
  }
}

// ====================================================================================
// keyboard_getchar: スキャンコードを ASCII 文字として取り出す 
// ====================================================================================
char keyboard_getchar(void) {
  int scancode;
  while ((scancode = keyboard_pop_scancode()) != KBD_SCANCODE_EMPTY) {
    if (scancode == KBD_RESP_ACK || scancode == KBD_RESP_RESEND) {
      continue;
    }

    // スキャンコードを ASCII 文字に変換
    char ch = scancode_to_ascii((unsigned char)scancode);

    // 文字に変換できたらコンソールに出力
    if (ch != 0) {
      return ch;
    }
  }

  return '\0'; // 入力文字なし
}













