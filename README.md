This is a shell as required for Assignment 2 of OSN.
It is divided into multiple files. For example, all built in commands required are implemented under builtin.c.
There are also header files including prototypes of each C file.
The program allows you to run various commands such as

- all system commands (this also supports sudo commands) handled in delegateCommands
- pwd, cd, echo, ls, discover

It allows you to run commands in background as well as foreground giving you the required information about the process.
It stores the last 20 commands in .history which can be accessed in commandshell by just typing `history`.

globalData.h handles all the prototypes and the global data that can be required across multiple files.
My discover command goes through hidden folders
The foreground timer returns the time required by the last foreground process that ran in the chain.
