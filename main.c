#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "colors.h"
#include "history.h"
#include "builtin.h"
#include "delegateCommands.h"
#include "globalData.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>

void printPrompt()
{
    char path[MAX_BUF];
    getcwd(path, MAX_BUF);

    char user[MAX_BUF];
    getlogin_r(user, MAX_BUF);

    char hostname[MAX_BUF];
    gethostname(hostname, MAX_BUF);

    char *homeDirInPath = strstr(path, shellHome);

    char newPath[MAX_BUF];
    if (homeDirInPath == path)
    {
        strcpy(newPath, "~");
        strcat(newPath, homeDirInPath + strlen(shellHome));
    }
    else
    {
        strcpy(newPath, path);
    }

    printf("<");
    green();
    bold();
    printf("%s@%s", user, hostname);
    reset();
    printf(":");
    blue();
    bold();
    printf("%s", newPath);
    reset();
    if (lastTime >= 1)
    {
        printf("took %.17gs", (double)lastTime);
    }
    printf("> ");
}

char *showPrompt()
{
    printPrompt();
    char *input = (char *)malloc(2000);
    fgets(input, 2000, stdin);
    return input;
}

void childDead()
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0)
    {
        int i;
        for (i = 0; i < curbackgroundJobs; i++)
        {
            if (backgroundJobs[i].pid == pid)
            {
                if (WEXITSTATUS(status) == 255)
                {
                    printf("\n%s: command not found\n", backgroundJobs[i].name);
                    printPrompt();
                    fflush(stdout);
                }
                else
                {
                    printf("\n%s with pid %d exited ", backgroundJobs[i].name, pid);
                    if (WIFEXITED(status))
                    {

                        if (WEXITSTATUS(status) == 0)
                        {
                            printf("normally.\n");
                        }
                        else
                        {
                            printf("abnormally with status = %d\n", WEXITSTATUS(status));
                        }

                        printPrompt();
                        fflush(stdout);
                    }
                    else if (WIFSIGNALED(status))
                    {
                        printf("abnormally with signal = %d\n", WTERMSIG(status));
                        fflush(stdout);
                        printPrompt();
                        fflush(stdout);
                    }
                    else if (WIFSTOPPED(status))
                    {
                        printf("abnormally with signal = %d\n", WSTOPSIG(status));
                        fflush(stdout);
                        printPrompt();
                        fflush(stdout);
                    }
                    else
                    {
                        printf("abnormally\n");
                        fflush(stdout);
                        printPrompt();
                        fflush(stdout);
                    }
                }

                for (int j = i; j < curbackgroundJobs; j++)
                {
                    backgroundJobs[j] = backgroundJobs[j + 1];
                }
                curbackgroundJobs--;
                break;
            }
        }
    }
    // clear output buffer
}

int main(void)
{
    signal(SIGCHLD, childDead);
    int exitFlag = 0;
    curbackgroundJobs = 0;
    // set current path as shell home malloc
    shellHome = (char *)malloc(MAX_BUF);
    getcwd(shellHome, MAX_BUF);

    readHistory();
    OLDPWD = NULL;

    while (!exitFlag)
    {
        char *in = showPrompt();
        lastTime = 0;
        addtoMemDirect(in);
        // add ; after every &
        char *inCopy = (char *)malloc(2000);
        int j = 0;
        for (int i = 0; i < strlen(in); i++)
        {
            if (in[i] == '&')
            {
                inCopy[j++] = '&';
                inCopy[j++] = ';';
            }
            else
            {
                inCopy[j++] = in[i];
            }
        }
        inCopy[j] = '\0';
        strcpy(in, inCopy);
        free(inCopy);

        // copy in
        char *input = (char *)malloc(strlen(in) + 1);
        strcpy(input, in);

        // count tokens
        int tokens = 0;
        char *token = strtok(input, ";");
        while (token != NULL)
        {
            tokens++;
            token = strtok(NULL, ";");
        }

        // create cmd array of strings malloc
        char **cmdArray = (char **)malloc(sizeof(char *) * tokens);
        if (tokens == 0)
        {
            continue;
        }
        cmdArray[0] = strtok(in, ";");
        // printf("%s\n", cmdArray[0]);
        for (int i = 1; i < tokens; i++)
        {
            cmdArray[i] = strtok(NULL, ";");
        }

        for (int i = 0; i < tokens; i++)
        {
            char *cmd = cmdArray[i];
            // copy
            char *cmdCopy = (char *)malloc(strlen(cmd) + 1);
            strcpy(cmdCopy, cmd);
            // count arguments
            int args = 0;
            char *arg = strtok(cmdCopy, " \t\n");
            while (arg != NULL)
            {
                args++;
                arg = strtok(NULL, " \t\n");
            }

            free(cmdCopy);

            // create arg array of strings malloc
            if (args == 0)
            {
                continue;
            }
            char **argArray = (char **)malloc(sizeof(char *) * (args + 1));
            argArray[0] = strtok(cmd, " \t\n");
            for (int j = 1; j < args; j++)
            {
                argArray[j] = strtok(NULL, " \t\n");
            }

            // addtoMem(argArray, args);

            if (strcmp(argArray[0], "exit") == 0)
            {
                exitFlag = 1;
                break;
            }
            else if (strcmp(argArray[0], "pwd") == 0)
            {
                pwd();
            }
            else if (strcmp(argArray[0], "echo") == 0)
            {
                echo(argArray, args);
            }
            else if (strcmp(argArray[0], "cd") == 0)
            {
                cd(argArray, args);
            }
            else if (strcmp(argArray[0], "ls") == 0)
            {
                ls(argArray, args);
            }
            else if (strcmp(argArray[0], "history") == 0)
            {
                printHistory();
            }
            else if (strcmp(argArray[0], "pinfo") == 0)
            {
                if (args == 1)
                {
                    pinfo(-1);
                }
                else
                {
                    pinfo(atoi(argArray[1]));
                }
            }
            else if (strcmp(argArray[0], "discover") == 0)
            {
                discover(argArray, args);
            }
            else
            {
                argArray[args] = NULL;
                if (!strcmp(argArray[args - 1], "&"))
                {
                    argArray[args - 1] = NULL;
                    delegate(argArray[0], argArray, 1);
                    lastTime = 0;
                }
                else
                {
                    lastTime = time(NULL);
                    delegate(argArray[0], argArray, 0);
                    lastTime = time(NULL) - lastTime;
                }
            }
            free(argArray);
        }

        free(cmdArray);

        free(in);
    }

    return 0;
}