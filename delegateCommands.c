#include "delegateCommands.h"
#include "globalData.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void delegate(char *command, char *args[], int background)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (background == 1)
            setpgid(0, 0);

        for (int i = 0; args[i] != NULL; i++)
        {
            char *temp = args[i];
            args[i] = parsePathforHome(args[i]);
        }

        int ret = execvp(command, args);
        exit(ret);
    }
    else
    {
        if (background == 0)
        {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                // printf("%d", WEXITSTATUS(status));
                if (WEXITSTATUS(status) == 255)
                {
                    printf("%s: command not found\n", command);
                }
            }
        }
        else
        {
            backgroundJobs[curbackgroundJobs].pid = pid;
            backgroundJobs[curbackgroundJobs].name = (char *)malloc(strlen(command) + 1);
            strcpy(backgroundJobs[curbackgroundJobs].name, command);
            printf("[%d] %d\n", ++curbackgroundJobs, pid);
        }
    }
}