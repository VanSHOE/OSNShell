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
            char *temp = parsePathforHome(args[i]);
            args[i] = (char *)malloc(strlen(temp) + 1);
            strcpy(args[i], temp);
            free(temp);
        }

        int ret = execvp(command, args);
        exit(ret);
    }
    else
    {
        // setpgid(pid, pid);
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

            // copy full command with args
            char *fullCmd = (char *)malloc(MAX_BUF);
            strcpy(fullCmd, command);
            for (int i = 1; args[i] != NULL; i++)
            {
                strcat(fullCmd, " ");
                strcat(fullCmd, args[i]);
            }
            backgroundJobs[curbackgroundJobs].cmd = fullCmd;

            printf("[%d] %d\n", ++curbackgroundJobs, pid);
        }
    }
}