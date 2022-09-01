#include "delegateCommands.h"
#include "globalData.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void delegate(char *command, char *args[], int background)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        int ret = execvp(command, args);
        exit(ret);
    }
    else
    {
        if (background == 0)
        {
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            printf("[%d] %d\n", ++curBackground, pid);
        }
    }
}