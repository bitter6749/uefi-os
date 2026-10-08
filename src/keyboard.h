#ifndef KEYBOARD_H
#define KEYBOARD_H

// --- 割り込みベクター・I/O ポート定義 ---
#define VEC_KEYBOARD      33      // 0x21: PS/2 キーボード割り込みベクタ
#define KBD_DATA_PORT     0x60    // PS/2 データポート
#define KBD_STATUS_PORT   0x64    // PS/2 ステータスレジスタ (Read Only)

// --- PS/2 ステータスレジスタ (0x64) ビットマスク ---
#define KBD_STATUS_OBF    0x01    // Bit 0: Output Buffer Full (1 = データ読出可)

// --- レスポンスコード定義 ---
#define KBD_RESP_ACK          0xFA    // Command Acknowledge
#define KBD_RESP_RESEND       (0xFE)  // Resend request
#define KBD_RESP_BAT_SUCCESS  0xAA    // Basic Assurance Test Passed

// --- 制御・バッファ定数 ---
#define KBD_BUFFER_SIZE       256     // リングバッファサイズ
#define KBD_SCANCODE_EMPTY    (-1)    // バッファが空の時の戻り値

// --- Scan Code Set 1 修飾キー定義 ---
#define SCANCODE_LSHIFT_MAKE    0x2A
#define SCANCODE_LSHIFT_BREAK   0xAA
#define SCANCODE_RSHIFT_MAKE    0x36
#define SCANCODE_RSHIFT_BREAK   0xB6

#define SCANCODE_BREAK_BIT      0x80  // Bit 7: 1 の場合は Break コード (キー離下)
#define SCANCODE_PREFIX_E0      0xE0  // 2バイト拡張キーのプレフィックス
#define SCANCODE_TABLE_MAX      0x40  // 変換テーブル有効範囲 (0x00 ~ 0x3F)

// --- キー状態値 ---
#define SHIFT_RELLEASED         0
#define SHIFT_PRESSED           1

// ====================================================================================
// キーボード・リングバッファ構造体
// ====================================================================================
typedef struct {
  unsigned char buffer[KBD_BUFFER_SIZE];
  volatile unsigned int head;   // 書き込みインデックス
  volatile unsigned int tail;   // 読み出しインデックス
} KeyBuffer;

// ====================================================================================
// keyboard_init: キーボードドライバおよび内蔵バッファの初期化を行う
// ====================================================================================
void keyboard_init(void);

// ====================================================================================
// c_keyboard_handler: キーボード割り込みハンドラ (isr33 アセンブリスタブから呼び出し)
// ====================================================================================
void c_keyboard_handler(void);

// ====================================================================================
// keyboard_pop_scancode: バッファからスキャンコードを 1 つ取り出す
// ====================================================================================
int keyboard_pop_scancode(void);  // バッファからスキャンコードを 1 つ取り出す (空なら -1)

// ====================================================================================
// keyboard_getchar: スキャンコードを ASCII 文字として取り出す 
// ====================================================================================
// - 戻り値:            ASCII コード (文字にならないキーは '\0' を返す)
char keyboard_getchar(void);

#endif
