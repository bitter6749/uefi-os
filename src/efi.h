// src/efi.h
#ifndef EFI_H
#define EFI_H

// UEFI の戻り地の型 (64-bit 符号なし整数)
typedef unsigned long long EFI_STATUS;
#define EFI_SUCCESS 0

// UEFI のハンドラおよびポインタ型 (最初は void* で十分)
typedef void *EFI_HANDLE; 

// GUID (プロトコルを一意に識別する128bit ID)
typedef struct {
  unsigned int   Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char  Data4[8];
} EFI_GUID;

// テーブルヘッダ (署名やサイズ情報)
typedef struct {
  unsigned long long Signature;
  unsigned int       Revision;
  unsigned int       HeaderSize;
  unsigned int       CRC32;
  unsigned int       Reserved;
} EFI_TABLE_HEADER;

// GOP 画面モード情報
typedef struct {
  unsigned int Version;
  unsigned int HorizontalResolution;  // 画面の横幅 (ピクセル)
  unsigned int VerticalResolution;    // 画面の縦幅 (ピクセル)
  int          PixelFormat;
  unsigned int RedMask;
  unsigned int GreenMask;
  unsigned int BlueMask;
  unsigned int ReservedMask;
  unsigned int PixelPerScanLine;     // 1行あたりのピクセル数 (パディング含む)
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

// GOP モード情報
typedef struct {
  unsigned int MaxMode;
  unsigned int Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  unsigned long long SizeOfInfo;
  unsigned long long FrameBufferBase; // VRAMの先頭アドレス
  unsigned long long FrameBufferSize; // VRAMのバイトサイズ
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

// GOP プロトコル本体
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
  void *QueryMode;
  void *SetMode;
  void *Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

// GOP の GUID: 9042a9de- 23dc-4a38-96fb-7aded080516a
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }}

// Boot Services: ファームウェアが提供する機能一覧
// ※なぜ使わない関数ポインタを大量に並べているのか？
// C言語の構造体は「メモリ上のオフセット(位置関係)」を決めるための型紙。
// UEFI仕様書において LocateProtocol は先頭から 320 バイトめに配置されている。
// その手前にある関数ポインタを同じ順番・個数でならべて位置 (オフセット) を正確に合わせている。
typedef struct {
  EFI_TABLE_HEADER Hdr;

  // ヘッダから LocateProtocol までのオフセットを合わせるための関数ポインタ群
  void *RaiseTPL;
  void *RestoreTPL;
  void *AllocatePages;
  void *FreePages;
  void *GetMemoryMap;
  void *AllocatePool;
  void *FreePool;
  void *CreateEvent;
  void *SetTimer;
  void *WaitForEvent;
  void *SignalEvent;
  void *CloseEvent;
  void *CheckEvent;
  void *InstallProtocolInterface;
  void *ReinstallProtocolInterface;
  void *UninstallProtocolInterface;
  void *HandleProtocol;
  void *Reserved;
  void *RegisterProtocolNotify;
  void *LocateHandle;
  void *LocateDevicePath;
  void *InstallConfigurationTable;
  void *LoadImage;
  void *StartImage;
  void *Exit;
  void *UnloadImage;
  void *ExitBootServices;
  void *GetNextMonotonicCount;
  void *Stall;
  void *SetWatchdogTimer;
  void *ConnectController;
  void *DisconnectController;
  void *OpenProtocol;
  void *CloseProtocol;
  void *OpenProtocolInformation;
  void *ProtocolPerHandle;
  void *LocateHandleBuffer;

  // 使用するプロトコル検索関数
  EFI_STATUS (*LocateProtocol)(EFI_GUID *Protocol, void *Registration, void **Interface);
} EFI_BOOT_SERVICES;

// System Table
typedef struct {
  EFI_TABLE_HEADER Hdr;
  void              *FirmwareVendor;
  unsigned int       FirmwareRevision;
  void              *ConsoleHandle;
  void              *ConIn;
  void              *ConsoleOutHandle;
  void              *ConOut;
  void              *StandardErrorHandle;
  void              *StdErr;
  void              *RuntimeServices;
  EFI_BOOT_SERVICES *BootServices; // ブートサービスへのポインタ
} EFI_SYSTEM_TABLE;

#endif

