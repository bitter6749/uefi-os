# uefi-os

UEFI (Unified Extensible Firmware Interface) 上で動作する 64-bit x86 自作オペレーティングシステムです。
**C言語** をメインとし、必要に応じて低レベルアセンブリを組み合わせて開発します。

---

## 📁 ディレクトリ構成

```text
.
├── Makefile          # ビルド・イメージ作成・QEMU実行・書き込み自動化
├── README.md         # 本ファイル
├── docs/             # 各種設計・環境構築ドキュメント
│   └── setup.md      # 開発環境構築ガイド
├── include/          # 共通ヘッダファイル (.h)
├── src/              # OS / カーネルのソースコード (.c, .asm)
└── build/            # ビルド生成物 (BOOTX64.EFI, disk.img など / gitignore対象)
```

---

## 🛠️ 必要環境・セットアップ

Ubuntu / Debian 系環境での構築を前提としています。

必要なパッケージ：
- `gcc-mingw-w64` (UEFI PE32+ 向け Cクロスコンパイラ `x86_64-w64-mingw32-gcc`)
- `binutils-mingw-w64` (PE32+ クロスリンカ `x86_64-w64-mingw32-ld`)
- `nasm` (低レベル制御用アセンブラ)
- `qemu-system-x86` (QEMU エミュレータ)
- `ovmf` (UEFI ファームウェア)
- `mtools` (FAT32 ディスクイメージ操作ツール)

詳細な手順については [docs/setup.md](docs/setup.md) を参照してください。

```bash
sudo apt update && sudo apt install -y gcc-mingw-w64 binutils-mingw-w64 nasm qemu-system-x86 ovmf mtools
```

---

## 🚀 ビルド & 実行方法

### 1. ビルド（EFIバイナリおよびFAT32ディスクイメージの作成）
`src/` 配下のソースコード（`.c` および `.asm`）をコンパイル・リンクし、`build/disk.img` (FAT32) を生成します。

```bash
make
```

生成物：
- `build/BOOTX64.EFI`: UEFI実行可能バイナリ (PE32+)
- `build/disk.img`: `\EFI\BOOT\BOOTX64.EFI` が格納されたブート用 FAT32 ディスクイメージ

### 2. QEMU による仮想マシン起動
OVMF UEFI ファームウェアを使用して QEMU 上で起動します。

```bash
make run
```

### 3. 実機用 USB メモリへの書き込み (`flash`)
USB メモリ等の外部ドライブにディスクイメージを直接書き込みます。

```bash
make flash DRIVE=/dev/sdX
```
> [!WARNING]
> 指定したドライブの全データが上書きされます。デバイス名（`/dev/sdX`）を必ず確認して実行してください。

### 4. クリーン
ビルド生成物を削除します。

```bash
make clean
```

---

## 🗺️ 開発ロードマップ

| Phase | テーマ | 主な内容 | GitHub Issue |
| :--- | :--- | :--- | :--- |
| **Phase 0** | **開発環境構築 & リポジトリ整備** | ドキュメント、Makefile (C/asm)、FAT32イメージ生成、QEMU連携 | #2 |
| **Phase 1** | **UEFIブート & GOP画面出力基盤** | C言語 UEFIエントリ、GOPフレームバッファ取得、フォント描画、コンソール | #3 |
| **Phase 2** | **メモリ管理 & CPUアーキテクチャ基盤** | メモリマップ解析、ページ割当、動的メモリ (`malloc`/`free`)、ページテーブル | #4 |
| **Phase 3** | **割り込み制御 & 時間管理** | IDT、例外ハンドラ、Local APIC タイマー | #5 |
| **Phase 4** | **キーボード入力 & シェル** | キーボードドライバ、スキャンコード解析、画面シェル | #6 |
| **Phase 5** | **PCI & ストレージ & ファイルシステム** | PCIスキャン、AHCI/SATAドライバ、FAT32ファイル読み書き | #7 |
| **Phase 6** | **マルチタスク & アプリケーション実行** | コンテキストスイッチ、Ring 3 ユーザーモード、システムコール | #8 |

---

## ✍️ ソースコード実装方針

`src/` 配下のソースコードは学習・理解のために手動で実装（模写）していきます。
C言語でのエントリポイントは `efi_main` です：

```c
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;

struct EFI_SYSTEM_TABLE; // UEFI System Table

EFI_STATUS efi_main(EFI_HANDLE image_handle, struct EFI_SYSTEM_TABLE *system_table) {
    // OS初期化処理
    return 0; // EFI_SUCCESS
}
```

- **コンパイラ**: `x86_64-w64-mingw32-gcc`
- **フラグ**: `-ffreestanding -fno-stack-protector -fno-stack-check -mno-red-zone -nostdlib`
- **低レイヤ処理**: アセンブリが必要な箇所は `src/*.asm` に配置すれば自動的にアセンブル・リンクされます。
