#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "colors.h"
#include "builtin.h"

#ifndef MAX_BUF
#define MAX_BUF 2000
#endif

char *showPrompt()
{
    char path[MAX_BUF];
    getcwd(path, MAX_BUF);

    char user[MAX_BUF];
    getlogin_r(user, MAX_BUF);

    char hostname[MAX_BUF];
    gethostname(hostname, MAX_BUF);

    char homeDir[MAX_BUF];
    strcpy(homeDir, "/home/");
    strcat(homeDir, user);

    // check if homedir in path
    char *homeDirInPath = strstr(path, homeDir);

    char newPath[MAX_BUF];
    if (homeDirInPath == path)
    {
        strcpy(newPath, "~");
        strcat(newPath, homeDirInPath + strlen(homeDir));
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
    while (!exitFlag)
    {
        char *in = showPrompt();
        // copy in
        char *input = (char *)malloc(strlen(in) + 1);
        strcpy(input, in);

        // count tokens
        int tokens = 0;
        char *token = strtok(input, ";&");
        while (token != NULL)
        {
            tokens++;
            token = strtok(NULL, ";&");
        }

        // create cmd array of strings malloc
        char **cmdArray = (char **)malloc(sizeof(char *) * tokens);
        cmdArray[0] = strtok(in, ";&");
        // printf("%s\n", cmdArray[0]);
        for (int i = 1; i < tokens; i++)
        {
            cmdArray[i] = strtok(NULL, ";&");
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
            char **argArray = (char **)malloc(sizeof(char *) * args);
            argArray[0] = strtok(cmd, " \t\n");
            for (int j = 1; j < args; j++)
            {
                argArray[j] = strtok(NULL, " \t\n");
            }

            for (int j = 0; j < args; j++)
            {
                printf("|%s|", argArray[j]);
            }
            printf("\n");

            continue;
            if (strcmp(in, "exit\n") == 0)
            {
                exitFlag = 1;
                break;
            }
            else if (strcmp(in, "pwd\n") == 0)
            {
                char path[MAX_BUF];
                getcwd(path, MAX_BUF);
                printf("%s\n", path);
            }
            else
            {
                printf("%s", in);
            }
        }

        free(in);
    }

    return 0;
}