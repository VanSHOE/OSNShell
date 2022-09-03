#include "builtin.h"
#include "globalData.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

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
    int result = 0;
    char cwd[MAX_BUF];
    getcwd(cwd, MAX_BUF);

    if (argc == 1)
    {
        result = chdir(shellHome);
    }
    else if (argc == 2)
    {
        if (strcmp(args[1], "-") == 0)
        {
            if (OLDPWD != NULL)
            {
                result = chdir(OLDPWD);
                if (result >= 0)
                {
                    printf("%s\n", OLDPWD);
                }
            }
            else
            {
                printf("OLDPWD is not set\n");
            }
        }
        else
        {
            char newPath[MAX_BUF];
            if (args[1][0] == '~')
            {
                strcpy(newPath, shellHome);
                strcat(newPath, args[1] + 1);
            }
            else
            {
                strcpy(newPath, args[1]);
            }
            result = chdir(newPath);
        }
    }
    else
    {
        printf("Too many arguments\n");
        return;
    }

    if (result == -1)
    {
        printf("cd: %s: No such file or directory\n", args[1]);
    }
    else
    {
        if (OLDPWD == NULL)
        {
            OLDPWD = (char *)malloc(MAX_BUF);
        }
        strcpy(OLDPWD, cwd);
    }
}

void ls(char *args[], int argc)
{
    if (argc > 4)
    {
        printf("Too many arguments\n");
        return;
    }
    else if (argc == 1)
    {
        DIR *dir = opendir(".");
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] != '.')
            {
                printf("%s\n", entry->d_name);
            }
        }
        closedir(dir);
    }
    else if (argc == 2)
    {
        if (strcmp(args[1], "-a") == 0)
        {
            DIR *dir = opendir(".");
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                printf("%s\n", entry->d_name);
            }
            closedir(dir);
        }
        else
        {
            printf("Invalid argument\n");
        }
    }
}

void pinfo(int pid)
{
    if (pid == -1)
    {
        pid = getpid();
    }

    char procStatPath[100];
    sprintf(procStatPath, "/proc/%d/stat", pid);
    FILE *procStat = fopen(procStatPath, "r");

    // check if file doesnt exist
    if (procStat == NULL)
    {
        printf("Process with pid %d does not exist\n", pid);
        return;
    }

    char state;
    unsigned long int mem;
    pid_t bgGrp, fgGrp;

    fscanf(procStat, "%*d %*s %c %*d %d %*d %*d %d %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %*u %lu", &state, &bgGrp, &fgGrp, &mem);
    fclose(procStat);

    char exePath[100];
    char procExePath[100];
    sprintf(procExePath, "/proc/%d/exe", pid);
    int len = readlink(procExePath, exePath, 100);
    exePath[len] = '\0';
    printf("pid : %d\nprocess Status : %c", pid, state);
    if (bgGrp == fgGrp)
    {
        // check if exists in background jobs
        int bg = 0;
        for (int i = 0; i < curbackgroundJobs; i++)
        {
            if (backgroundJobs[i].pid == pid)
            {
                bg = 1;
                break;
            }
        }

        if (!bg)
            printf("+");
    }
    char *homeDirInPath = strstr(exePath, shellHome);

    char newPath[MAX_BUF];
    if (homeDirInPath == exePath)
    {
        strcpy(newPath, "~");
        strcat(newPath, homeDirInPath + strlen(shellHome));
    }
    else
    {
        strcpy(newPath, exePath);
    }
    printf("\nmemory : %lu\nexecutable Path : %s\n", mem, newPath);
}