#include "builtin.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void pwd()
{
    char path[200];
    getcwd(path, 200);
    printf("%s\n", path);
}

void echo(char *args[], int argc)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", args[i]);
    }

    printf("\n");
}

void cd(char *args[], int argc)
{
    if (argc == 1)
    {
        printf("cd: missing operand\n");
        chdir(getenv("HOME"));
    }
    else
    {
        printf("Moving to: %s\n", args[1]);
        chdir(args[1]);
    }
}