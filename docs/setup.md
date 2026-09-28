# 開発環境構築ガイド

本プロジェクトでは、UEFI (Unified Extensible Firmware Interface) 上で動作する64ビットオペレーティングシステムを、主に **C言語**（および必要に応じた低レベルアセンブリ）を用いてゼロから開発します。
開発環境としては、Ubuntu / Debian 系の Linux 環境を想定しています。

---

## 1. 必要なツール・パッケージ一覧

| パッケージ名 | 用途・役割 |
| :--- | :--- |
| `gcc-mingw-w64` | UEFI アプリケーション（Windows PE32+ 形式）向けのクロス C コンパイラ (`x86_64-w64-mingw32-gcc`) を提供します。 |
| `binutils-mingw-w64` | Windows PE32+ 形式のバイナリをリンクするためのクロスリンカ (`x86_64-w64-mingw32-ld`) を提供します。 |
| `nasm` | x86_64 アセンブラ。低レベルCPU制御やスタブなどのアセンブリコード (`.asm`) をコンパイルします。 |
| `qemu-system-x86` | x86_64 アーキテクチャのエミュレータ (`qemu-system-x86_64`)。仮想環境で OS を起動・デバッグします。 |
| `ovmf` | QEMU で UEFI ブートを可能にするオープンソースの UEFI ファームウェア (Open Virtual Machine Firmware)。 |
| `mtools` | Linux 上から直接 FAT ファイルシステム（ディスクイメージ）を操作・編集するツール群 (`mformat`, `mmd`, `mcopy` など)。 |

---

## 2. インストール手順

以下のコマンドを実行して必要なパッケージを一括インストールします。

```bash
sudo apt update
sudo apt install -y \
    gcc-mingw-w64 \
    binutils-mingw-w64 \
    nasm \
    qemu-system-x86 \
    ovmf \
    mtools
```

---

## 3. インストール確認

各コマンドが正しくインストールされているか確認します。

```bash
# C コンパイラ & リンカ & アセンブラ
x86_64-w64-mingw32-gcc -v
x86_64-w64-mingw32-ld -v
nasm -v

# エミュレータ
qemu-system-x86_64 --version

# FAT操作ツール
mcopy -V

# OVMF ファームウェアの存在確認
ls -l /usr/share/ovmf/OVMF.fd || ls -l /usr/share/qemu/OVMF.fd
```

---

## 4. OVMF パスのカスタマイズ（必要な場合のみ）

Makefile は自動的に以下のパスから OVMF ファームウェアを探索します：
1. `/usr/share/ovmf/OVMF.fd`
2. `/usr/share/qemu/OVMF.fd`
3. `/usr/share/OVMF/OVMF_CODE_4M.fd`
4. `/usr/share/OVMF/OVMF.fd`
5. `/usr/share/edk2-ovmf/x64/OVMF.fd`

もし別の場所に配置している場合は、環境変数または make 実行時に指定できます：

```bash
make run OVMF=/path/to/your/OVMF.fd
```
