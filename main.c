#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <dirent.h>
#include "colors.h"
#include "history.h"
#include "builtin.h"
#include "delegateCommands.h"
#include "globalData.h"

char *showPrompt()
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
    printf("> ");

    char *input = (char *)malloc(2000);
    fgets(input, 2000, stdin);
    return input;
}

int main(void)
{
    int exitFlag = 0;
    curBackground = 0;
    // set current path as shell home malloc
    shellHome = (char *)malloc(MAX_BUF);
    getcwd(shellHome, MAX_BUF);

    readHistory();
    OLDPWD = NULL;

    while (!exitFlag)
    {
        char *in = showPrompt();
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

            addtoMem(argArray, args);

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
                // list files
                DIR *d;
                struct dirent *dir;
                d = opendir(".");
                if (d)
                {
                    while ((dir = readdir(d)) != NULL)
                    {
                        printf("%s\n", dir->d_name);
                    }
                    closedir(d);
                }
            }
            else
            {
                argArray[args] = NULL;
                if (!strcmp(argArray[args - 1], "&"))
                {
                    delegate(argArray[0], argArray, 1);
                }
                else
                {
                    delegate(argArray[0], argArray, 0);
                }
            }
        }

        free(in);
    }

    return 0;
}