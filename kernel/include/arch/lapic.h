#ifndef LAPIC_H
#define LAPIC_H

// ============================================================================
// Local APIC (Advanced Programmable Interrupt Controller) Timer の概要
// ============================================================================
// Local APIC タイマーは、x86_64 CPU の各コア内に内蔵された高精度タイマーです。
// OS の「時の流れ (Tick 管理)」や「将来的なマルチタスク (タスク切り替え)」の
// 基準となる定期割り込み (周期タイマー) を生成するために使用します。
//
// ----------------------------------------------------------------------------
// 【動作メカニズム (Periodic モード)】
// ----------------------------------------------------------------------------
// 1. Initial Count Register にカウント初期値 (例: 10000000) を設定してスタート。
// 2. タイマーは CPU バス時計 (分周後) に同期して 0 へ向かってカウントダウン。
// 3. カウントが 0 に到達すると、指定した割り込みベクター (Vector 32: 0x20) を発行。
// 4. Periodic モードの場合、0 に到達した瞬間に自動的に Initial Count の値が
//    再ロードされ、カウントダウンを永久に繰り返す (メトロノームの役割)。
//
// [ Local APIC Timer ] ──( 1ms ごとに割り込み )──▶ [ CPU コア ]
//                                                    │
//                                                    ▼
//                                      [ 割り込みハンドラ (isr32) ]
//                                      ・g_ticks を +1 インクリメント
//                                      ・lapic_eoi() で完了通知 (必須)
//
// ----------------------------------------------------------------------------
// 【重要: EOI (End of Interrupt) 通知】
// ----------------------------------------------------------------------------
// 割り込みハンドラの最後で、必ず Local APIC の EOI レジスタ (0xFEE000B0) に
// 0 を書き込む必要があります。これを怠ると Local APIC は「前の割り込みを処理中」
// と判断し、2 回目以降のタイマー割り込みを送らなくなります。
// ============================================================================

// 割り込みベクター番号定義 (0 ~ 31 は CPU 例外のため、 32 以降を使用)
#define VEC_TIMER                 32    // 0x20: local APIC タイマー用割り込み

// ============================================================================
// Local APIC レジスタオフセット定義 (MMIO)
// ============================================================================
#define LAPIC_REG_ID              0x0020  // APIC ID レジスタ
#define LAPIC_REG_EOI             0x00B0  // EOI (End of Interrupt) レジスタ
#define LAPIC_REG_SPURIOUS        0x00F0  // Spurious Interrupt Vector レジスタ
#define LAPIC_REG_LVT_TIMER       0x0320  // LVT タイマー設定レジスタ
#define LAPIC_REG_TIMER_INIT      0x0380  // タイマー初期カウントレジスタ
#define LAPIC_REG_TIMER_CURR      0x0390  // タイマー現在カウントレジスタ
#define LAPIC_REG_TIMER_DIV       0x03E0  // タイマー分周設定レジスタ

// ============================================================================
// Local APIC レジスタ設定用フラグ・定数
// ============================================================================
#define LAPIC_BASE_ADDR           0xFEE00000ULL // Local APIC MMIO ベース物理アドレス (標準値)
#define LAPIC_BASE_MASK           0xFFFFF000ULL // MSR から物理アドレスを抽出するマスク

#define LAPIC_SPURIOUS_ENABLE     0x100         // APIC ソフトウェア有効化フラグ (Bit 8)
#define LAPIC_SPURIOUS_VECTOR     0xFF          // スプリアス割り込み用ダミーベクタ

#define LAPIC_TIMER_PERIODIC      0x20000       // タイマー Periodic (周期) モードフラグ (Bit 17)
#define LAPIC_TIMER_DIV_16        0x03          // 分周比: 1/16 (Bit 0,1,3 で設定)

#define LAPIC_TIMER_INIT_COUNT    1000000       // 1ms ~ 数ms 周期のカウント初期値

// --- MSR (Model Specific Register) 定数 ---
#define MSR_IA32_APIC_BASE        0x1B          // APIC ベースアドレス取得用 MSR

// ============================================================================
// 関数プロトタイプ宣言
// ============================================================================

// Local APIC の初期化および Periodic タイマーの開始
void lapic_timer_init(void);

// 割り込み処理完了通知 (EOI: End of Interrupt) を Local APIC に送信
void lapic_eoi(void);

// 経過ティック数 (ms 単位等のタイムカウント) の取得
unsigned long long lapic_get_ticks(void);

#endif
