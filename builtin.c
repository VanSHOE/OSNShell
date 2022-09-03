#include "builtin.h"
#include "globalData.h"
#include "colors.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int isFlag(char *arg)
{
    if (arg[0] == '-')
    {
        return 1;
    }
    return 0;
}

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

void discoverRecurse(char *path, int allFile, int allDir, char *name)
{
    int nameMatch = name == NULL ? 1 : 0;
    if (name != NULL)
    {
        int i = strlen(path) - 1;
        while (path[i] != '/' && i != 0)
        {
            i--;
        }

        if (path[i] == '/')
        {
            i++;
        }

        if (strcmp(path + i, name) == 0)
        {
            nameMatch = 1;
        }
    }

    DIR *dir = opendir(path);
    struct dirent *entry;

    if (dir == NULL)
    {
        if (allFile && nameMatch)
        {
            printf("%s\n", path);
        }

        return;
    }
    if (allDir && nameMatch)
    {
        printf("%s\n", path);
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char *newPath = (char *)malloc(MAX_BUF);
        strcpy(newPath, path);
        strcat(newPath, "/");
        strcat(newPath, entry->d_name);
        discoverRecurse(newPath, allFile, allDir, name);
        free(newPath);
    }
}

void discover(char *args[], int argc)
{
    // equivalent of find command
    if (argc > 5)
    {
        printf("Too many arguments\n");
        return;
    }

    int anyFlag = 0;
    int allFile = 0;
    int allDir = 0;
    char *name = NULL;
    char *path = (char *)malloc(MAX_BUF);
    strcpy(path, ".");

    for (int i = 1; i < argc; i++)
    {
        if (isFlag(args[i]))
        {
            anyFlag = 1;
            if (strcmp(args[i], "-f") == 0)
            {
                allFile = 1;
            }
            else if (strcmp(args[i], "-d") == 0)
            {
                allDir = 1;
            }
            else if (strcmp(args[i], "-df") == 0 || strcmp(args[i], "-fd") == 0)
            {
                allDir = 1;
                allFile = 1;
            }
            else
            {
                printf("Invalid flag(s)\n");
                return;
            }
            continue;
        }

        // check if dir
        DIR *dir = opendir(args[i]);
        if (dir != NULL)
        {
            // path = args[i];
            strcpy(path, args[i]);
            closedir(dir);
            continue;
        }

        // check if name
        if (name == NULL)
        {
            name = (char *)malloc(MAX_BUF);
            // name = args[i];
            strcpy(name, args[i]);
            continue;
        }

        printf("Invalid argument: %s\n", args[i]);
    }
    if (!anyFlag)
    {
        allFile = 1;
        allDir = 1;
    }
    // if name has quotes, remove
    char *rectifiedName = (char *)malloc(MAX_BUF);
    if (name != NULL && name[0] == '"' && name[strlen(name) - 1] == '"')
    {

        for (int i = 1; i < strlen(name) - 1; i++)
        {
            rectifiedName[i - 1] = name[i];
        }
        rectifiedName[strlen(name) - 2] = '\0';
    }
    else
    {
        strcpy(rectifiedName, name);
    }

    discoverRecurse(path, allFile, allDir, rectifiedName);
    free(name);
    free(rectifiedName);
    free(path);
}

int isExecutable(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode) && (path_stat.st_mode & S_IXUSR);
}

int isDir(char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

void ls(char *args[], int argc)
{
    int dirCount = 0;
    int fileCount = 0;

    char *dirs[argc];
    char *files[argc];

    int l = 0;
    int a = 0;

    for (int i = 1; i < argc; i++)
    {
        if (isFlag(args[i]))
        {
            for (int j = 1; j < strlen(args[i]); j++)
            {
                if (args[i][j] == 'l')
                {
                    l = 1;
                }
                else if (args[i][j] == 'a')
                {
                    a = 1;
                }
                else
                {
                    printf("Invalid flag: %c\n", args[i][j]);
                    return;
                }
            }

            continue;
        }

        DIR *dir = opendir(args[i]);
        if (dir != NULL)
        {
            dirs[dirCount] = (char *)malloc(MAX_BUF);
            strcpy(dirs[dirCount], args[i]);
            dirCount++;
            closedir(dir);
        }
        else
        {
            FILE *file = fopen(args[i], "r");
            if (file != NULL)
            {
                files[fileCount] = (char *)malloc(MAX_BUF);
                strcpy(files[fileCount], args[i]);
                fileCount++;
                fclose(file);
            }
            else
            {
                printf("ls: cannot access '%s': No such file or directory\n", args[i]);
                return;
            }
        }
    }

    if (dirCount == 0)
    {
        dirs[dirCount] = (char *)malloc(MAX_BUF);
        strcpy(dirs[dirCount], ".");
        dirCount++;
    }

    if (l == 0)
    {
        for (int i = 0; i < dirCount; i++)
        {
            if (dirCount != 1)
                printf("%s:\n", dirs[i]);

            DIR *dir = opendir(dirs[i]);
            struct dirent *entry;

            while ((entry = readdir(dir)) != NULL)
            {
                if (a == 0 && entry->d_name[0] == '.')
                {
                    continue;
                }
                if (isDir(entry->d_name))
                {

                    blue();
                    bold();
                }
                else if (isExecutable(entry->d_name))
                {

                    green();
                    bold();
                }

                printf("%s\n", entry->d_name);
                reset();
            }

            closedir(dir);
            if (i != dirCount - 1)
                printf("\n");
        }
    }
    else
    {
    }

    for (int i = 0; i < dirCount; i++)
    {
        free(dirs[i]);
    }
    for (int i = 0; i < fileCount; i++)
    {
        free(files[i]);
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
        // // check if exists in background jobs
        // int bg = 0;
        // for (int i = 0; i < curbackgroundJobs; i++)
        // {
        //     if (backgroundJobs[i].pid == pid)
        //     {
        //         bg = 1;
        //         break;
        //     }
        // }

        // if (!bg)
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