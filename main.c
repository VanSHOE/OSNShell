#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "colors.h"

#ifndef MAX_BUF
#define MAX_BUF 200
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
    while (1)
    {
        char *in = showPrompt();
        if (strcmp(in, "exit\n") == 0)
        {
            break;
        }
        else
        {
            printf("%s", in);
        }
        free(in);
    }
    return 0;
}