#include "shell/shell.h"
#include "shell/command.h"

DEFINE_COMMAND(help);
DEFINE_COMMAND(clear);
DEFINE_COMMAND(echo);
DEFINE_COMMAND(ticks);
DEFINE_COMMAND(exit);
DEFINE_COMMAND(lspci);
DEFINE_COMMAND(ahci);

// 一覧テーブル
const ShellCommand g_shell_commands[] = {
  {"help",  cmd_help,   "Display avaliable commands"},
  {"clear", cmd_clear,  "Clear the console screen"},
  {"echo",  cmd_echo,   "Display a line of text"},
  {"ticks", cmd_ticks,  "Display CPU/Timer tick count"},
  {"exit",  cmd_exit,   "Shutdown the system"},
  {"lspci", cmd_lspci,  "List all PCI devices"},
  {"ahci",  cmd_ahci,   "Show AHCI controller info"},
};

const unsigned int g_shell_commands_count = 
    sizeof(g_shell_commands) / sizeof(g_shell_commands[0]);

