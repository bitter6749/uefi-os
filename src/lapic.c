#include "lapic.h"

#define INT_SIZE  32

// 経過ティック数 (ミリ秒単位等の時間管理用カウンタ)
static volatile unsigned long long g_ticks = 0;

// 動的に取得した Local APIC のベース物理アドレスを保持する変数
static unsigned long long g_lapic_base = 0;

// ============================================================================
// MMIO レジスタアクセス関数 
// ============================================================================
static inline unsigned int lapic_read(unsigned int reg) {
  return *(volatile unsigned int *)(g_lapic_base + reg);
}

static inline void lapic_write(unsigned int reg, unsigned int val) {
  *(volatile unsigned int *)(g_lapic_base + reg) = val;
}

// ============================================================================
// IA32_APIC_BASE MSR を読み取る関数
// ============================================================================
unsigned long long get_lapic_base_msr(void) {
  unsigned int low, high;
  // rdmsr 命令で MSR を読み出す
  // 上位32bitが high に 下位32bitが low に格納される
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(MSR_IA32_APIC_BASE));

  unsigned long long msr = ((unsigned long long)high << INT_SIZE) | low;

  //  0 ~ 7   bit 予約領域
  //    8     bit BSPフラグ (1なら最初に起動したのCPUのコア、0ならサブコア)  
  //  9 ~ 10  bit 予約領域
  //    11    bit APIC Global Enable フラグ (1でローカルAPICが有効、0で無効)
  // 12 ~ 35  bit Local APIC のベース物理アドレスが格納されている
  //    ~ 63  bit 予約領域
  return msr & LAPIC_BASE_MASK;
}

// ============================================================================
// lapic_eoi: 割り込み処理完了通知 (EOI) を Local APIC に送信する
// ============================================================================
void lapic_eoi(void) {
  lapic_write(LAPIC_REG_EOI, 0);
}

// ============================================================================
// lapic_get_ticks: 累積ティック数を取得する
// ============================================================================
unsigned long long lapic_get_ticks(void) {
  return g_ticks;
}

// ============================================================================
// c_timer_handler: タイマー割り込みハンドラ (isr32 から呼び出される)
// ============================================================================
void c_timer_handler(void) {
  g_ticks++;

  // 処理完了を Local APIC に通知 (これを忘れると次の割り込みが入らない)
  lapic_eoi();
}

// ============================================================================
// lapic_timer_init: Local APIC を有効化し Periodic タイマーを開始する
// ============================================================================
void lapic_timer_init(void) {
  // 1. MSR から実際の LAPIC ベース物理アドレスを取得
  g_lapic_base = get_lapic_base_msr();

  if (g_lapic_base == 0) {
    g_lapic_base = LAPIC_BASE_ADDR; // 0xFEE00000ULL
  }

  // 1. Local APIC 全体の有効化 (Spurious Interrupt Vector レジスタの設定)
  //    Bit 8 を 1 にして APIC を有効化 + ダミー割り込み用に Vector 0xFF (255) を設定
  lapic_write(LAPIC_REG_SPURIOUS, lapic_read(LAPIC_REG_SPURIOUS) | LAPIC_SPURIOUS_ENABLE | LAPIC_SPURIOUS_VECTOR);

  // 2. タイマー分周比の設定 (1/16 分周)
  lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);

  // 3. LVT タイマーレジスタの設定
  //    Periodic (分周) モードを有効化し、割り込みベクターとして VEC_TIMER (32) を割り当て
  lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_PERIODIC | VEC_TIMER);

  // 4. カウント初期値の設定 (値を書き込んだ瞬間にカウントダウンが始動)
  //    環境やクロック速度に合わせて数値を調整する
  lapic_write(LAPIC_REG_TIMER_INIT, LAPIC_TIMER_INIT_COUNT);

  // 5. CPU の割り込み受信を許可 (sti 命令)
  __asm__ volatile("sti");
}





















