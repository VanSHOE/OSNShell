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
        setpgid(pid, pid);
        if (background == 0)
        {
            int status;
            foregroundJobs[curforegroundJobs].pid = pid;
            foregroundJobs[curforegroundJobs].name = (char *)malloc(strlen(command) + 1);
            strcpy(foregroundJobs[curforegroundJobs].name, command);

            // copy full command with args
            char *fullCmd = (char *)malloc(MAX_BUF);
            strcpy(fullCmd, command);
            for (int i = 1; args[i] != NULL; i++)
            {
                strcat(fullCmd, " ");
                strcat(fullCmd, args[i]);
            }

            foregroundJobs[curforegroundJobs++].cmd = fullCmd;
            // printf("Waiting for: %d\n", pid);

            signal(SIGTTOU, SIG_IGN);
            signal(SIGTTIN, SIG_IGN);
            tcsetpgrp(STDIN_FILENO, getpgid(pid));
            tcsetpgrp(STDOUT_FILENO, getpgid(pid));

            waitpid(pid, &status, WUNTRACED);

            tcsetpgrp(STDIN_FILENO, getpgrp());
            tcsetpgrp(STDOUT_FILENO, getpgrp());
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            // check if sigstopped
            if (!WIFSTOPPED(status))
            {
                for (int i = 0; i < curforegroundJobs; i++)
                {
                    if (foregroundJobs[i].pid == pid)
                    {
                        free(foregroundJobs[i].name);
                        free(foregroundJobs[i].cmd);
                        for (int j = i; j < curforegroundJobs - 1; j++)
                        {
                            foregroundJobs[j] = foregroundJobs[j + 1];
                        }
                        curforegroundJobs--;
                        break;
                    }
                }
            }
            else
            {
                for (int i = 0; i < curforegroundJobs; i++)
                {
                    setpgid(foregroundJobs[i].pid, 0);
                    kill(foregroundJobs[i].pid, SIGTTIN);
                    kill(foregroundJobs[i].pid, SIGTSTP);

                    backgroundJobs[curbackgroundJobs++] = foregroundJobs[i];
                    for (int j = i; j < curforegroundJobs - 1; j++)
                    {
                        foregroundJobs[j] = foregroundJobs[j + 1];
                    }
                    curforegroundJobs--;
                }
                curforegroundJobs = 0;
                printf("\n");
                printPrompt();

                fflush(stdout);
            }
            // printf("Done waiting");
            // printf("%d", WEXITSTATUS(status));
            if (WIFEXITED(status))
            {
                // printf("%d", WEXITSTATUS(status));
                // printf("Add");
                // delete from fg

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