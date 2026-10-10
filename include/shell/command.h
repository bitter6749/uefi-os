#ifndef SHELL_COMMAND_H
#define SHELL_COMMAND_H

#include "shell/shell.h"

// コマンド関数の命名とシグネチャをマクロで統一
#define DEFINE_COMMAND(name) \
    void cmd_##name(KernelShell *shell, int argc, char **argv)

// 外部参照用コマンドテーブル
extern const ShellCommand g_shell_commands[];
extern const unsigned int g_shell_commands_count;

#endif
