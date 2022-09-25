This is a shell as required for Assignment 2 and 3 of OSN.
It is divided into multiple files. For example, all built in commands required are implemented under builtin.c.
There are also header files including prototypes of each C file.
The program allows you to run various commands such as

- Features a tab auto-completion
- Signal handling such as Ctrl C, Z, D
- Piping and redirection supported
- all system commands handled in delegateCommands
- `pwd`, `cd`, `echo`, `ls`, `discover` (like `find`)
- `jobs` (display shell's bg jobs), `sig` (send a signal to a child process), `fg` (bring bg process to fg and resume), `bg` (resume process in bg)

It allows you to run commands in background as well as foreground giving you the required information about the process.
It stores the last 20 commands in .history which can be accessed in commandshell by just typing `history`.

globalData.h handles all the prototypes and the global data that can be required across multiple files.
My discover command goes through hidden folders
The foreground timer returns the time required by the last foreground process that ran in the chain.
