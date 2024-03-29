#include "builtin.h"
#include "globalData.h"
#include "colors.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "history.h"
#include <signal.h>

int callInbuilt(char *argArray[], int args)
{
    int result = 1;
    if (strcmp(argArray[0], "exit") == 0)
    {
        exitFlag = 1;
    }
    else if (strcmp(argArray[0], "pwd") == 0)
    {
        pwd();
    }
    else if (strcmp(argArray[0], "jobs") == 0)
    {
        jobs(argArray, args);
    }
    else if (strcmp(argArray[0], "echo") == 0)
    {
        echo(argArray, args);
    }
    else if (strcmp(argArray[0], "cd") == 0)
    {
        cd(argArray, args);
    }
    else if (strcmp(argArray[0], "sig") == 0)
    {
        sendSignal(argArray, args);
    }
    else if (strcmp(argArray[0], "ls") == 0)
    {
        ls(argArray, args);
    }
    else if (strcmp(argArray[0], "bg") == 0)
    {
        resumeBG(argArray, args);
    }
    else if (strcmp(argArray[0], "fg") == 0)
    {
        bringFG(argArray, args);
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
        result = 0;
    }

    return result;
}

char *parsePathforHome(char *path)
{
    if (strlen(path) == 0)
        return path;

    char *newPath = (char *)malloc(sizeof(char) * (strlen(path) + strlen(shellHome) + 1));
    // printf("Path in parse: %s", path);
    if ((strlen(path) == 1 && path[0] == '~') || (path[0] == '~' && path[1] == '/'))
    {
        strcpy(newPath, shellHome);
        strcat(newPath, path + 1);
    }
    else
    {
        strcpy(newPath, path);
    }
    // printf("What is happening...: %s", newPath);
    return newPath;
}

char *reverseParsePath(char *path)
{
    char *homeDirInPath = strstr(path, shellHome);

    char *newPath = (char *)malloc(strlen(path) + 1);
    if (homeDirInPath == path)
    {
        strcpy(newPath, "~");
        strcat(newPath, homeDirInPath + strlen(shellHome));
    }
    else
    {
        strcpy(newPath, path);
    }

    return newPath;
}

int lsCmp(const void *a, const void *b)
{
    // // check for . and ..
    // if (strcmp(((struct lsLEntry *)a)->name, ".") == 0)
    //     return -1;
    // if (strcmp(((struct lsLEntry *)b)->name, ".") == 0)
    //     return 1;
    // if (strcmp(((struct lsLEntry *)a)->name, "..") == 0 && strcmp(((struct lsLEntry *)b)->name, ".") != 0)
    //     return -1;
    // if (strcmp(((struct lsLEntry *)b)->name, "..") == 0 && strcmp(((struct lsLEntry *)a)->name, ".") != 0)
    //     return 1;

    // lower case both strings
    char *aLower = (char *)malloc(strlen(((struct lsLEntry *)a)->name) + 1);
    char *bLower = (char *)malloc(strlen(((struct lsLEntry *)b)->name) + 1);

    for (int i = 0; i < strlen(((struct lsLEntry *)a)->name); i++)
        aLower[i] = tolower(((struct lsLEntry *)a)->name[i]);
    aLower[strlen(((struct lsLEntry *)a)->name)] = '\0';

    for (int i = 0; i < strlen(((struct lsLEntry *)b)->name); i++)
        bLower[i] = tolower(((struct lsLEntry *)b)->name[i]);
    bLower[strlen(((struct lsLEntry *)b)->name)] = '\0';

    int ans = strcmp(aLower, bLower);
    free(aLower);
    free(bLower);
    return ans;
}

int lsStringCmp(const void *a, const void *b)
{
    // lower both
    char *aLower = (char *)malloc(strlen((char *)a) + 1);
    char *bLower = (char *)malloc(strlen((char *)b) + 1);

    for (int i = 0; i < strlen((char *)a); i++)
        aLower[i] = tolower(((char *)a)[i]);
    aLower[strlen((char *)a)] = '\0';

    for (int i = 0; i < strlen((char *)b); i++)
        bLower[i] = tolower(((char *)b)[i]);
    bLower[strlen((char *)b)] = '\0';

    int ans = strcmp(aLower, bLower);
    free(aLower);
    free(bLower);
    return ans;
}

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
    closedir(dir);
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

        if (name == NULL && args[i][0] == '"' && args[i][strlen(args[i]) - 1] == '"')
        {
            name = (char *)malloc(MAX_BUF);
            strcpy(name, args[i]);
            continue;
        }

        // check if dir
        char *rectifiedPath = parsePathforHome(args[i]);
        DIR *dir = opendir(rectifiedPath);
        // printf("%s\n", args[i]);
        // fflush(stdout);
        if (dir != NULL)
        {
            strcpy(path, rectifiedPath);
            free(rectifiedPath);
            closedir(dir);
            continue;
        }
        free(rectifiedPath);

        // check if name

        printf("Invalid argument: %s\n", args[i]);
        return;
    }
    if (!anyFlag)
    {
        allFile = 1;
        allDir = 1;
    }
    // if name has quotes, remove
    // printf("allFile: %d, allDir: %d, name: %s, path: %s\n", allFile, allDir, name, path);
    // fflush(stdout);
    char *rectifiedName = (char *)malloc(MAX_BUF);
    if (name != NULL && strlen(name) >= 1 && name[0] == '"' && name[strlen(name) - 1] == '"')
    {

        for (int i = 1; i < strlen(name) - 1; i++)
        {
            rectifiedName[i - 1] = name[i];
        }
        rectifiedName[strlen(name) - 2] = '\0';
    }
    else
    {
        if (name != NULL)
            strcpy(rectifiedName, name);
        else
            rectifiedName = NULL;
    }
    // print all args
    // printf("allFile: %d, allDir: %d, name: %s, path: %s\n", allFile, allDir, rectifiedName, path);
    // fflush(stdout);
    discoverRecurse(path, allFile, allDir, rectifiedName);
    if (name != NULL)
        free(name);
    if (rectifiedName != NULL)
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

int getDigits(int x)
{
    if (x == 0)
        return 1;

    int digits = 0;
    while (x != 0)
    {
        x /= 10;
        digits++;
    }
    return digits;
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

        char *rectifiedPath = parsePathforHome(args[i]);
        DIR *dir = opendir(rectifiedPath);
        if (dir != NULL)
        {
            dirs[dirCount] = (char *)malloc(MAX_BUF);
            strcpy(dirs[dirCount], rectifiedPath);
            dirCount++;
            closedir(dir);
        }
        else
        {
            FILE *file = fopen(rectifiedPath, "r");
            if (file != NULL)
            {
                files[fileCount] = (char *)malloc(MAX_BUF);
                strcpy(files[fileCount], rectifiedPath);
                fileCount++;
                fclose(file);
            }
            else
            {
                printf("ls: cannot access '%s': No such file or directory\n", rectifiedPath);
                return;
            }
        }
        free(rectifiedPath);
    }

    if (dirCount == 0 && fileCount == 0)
    {
        dirs[dirCount] = (char *)malloc(MAX_BUF);
        strcpy(dirs[dirCount], ".");
        dirCount++;
    }

    if (l == 0)
    {
        qsort(files, fileCount, sizeof(char *), lsStringCmp);
        for (int i = 0; i < fileCount; i++)
        {
            printf("%s\n", files[i]);
        }

        char **dirFiles = (char **)malloc(dirCount * sizeof(char *));
        for (int i = 0; i < dirCount; i++)
        {

            if (dirCount != 1 || fileCount)
                printf("%s:\n", dirs[i]);

            DIR *dir = opendir(dirs[i]);
            struct dirent *entry;

            int count = 0;
            while ((entry = readdir(dir)) != NULL)
            {
                if (a == 0 && entry->d_name[0] == '.')
                {
                    continue;
                }
                count++;
            }
            closedir(dir);

            dir = opendir(dirs[i]);

            struct lsLEntry *lsEntries = (struct lsLEntry *)malloc(count * sizeof(struct lsLEntry));
            int index = 0;
            while ((entry = readdir(dir)) != NULL)
            {
                if (a == 0 && entry->d_name[0] == '.')
                {
                    continue;
                }
                char *pathToFile = (char *)malloc(MAX_BUF);
                strcpy(pathToFile, dirs[i]);
                strcat(pathToFile, "/");
                strcat(pathToFile, entry->d_name);
                lsEntries[index].name = (char *)malloc(strlen(entry->d_name) + 1);
                strcpy(lsEntries[index].name, entry->d_name);
                lsEntries[index].path = (char *)malloc(strlen(pathToFile) + 1);
                strcpy(lsEntries[index].path, pathToFile);

                if (isDir(pathToFile))
                {

                    blue();
                    bold();
                }
                else if (isExecutable(pathToFile))
                {

                    green();
                    bold();
                }

                // printf("%s\n", entry->d_name);
                dirFiles[i] = (char *)malloc(strlen(entry->d_name) + 1);
                strcpy(dirFiles[i], entry->d_name);

                reset();
                index++;
            }
            qsort(lsEntries, count, sizeof(struct lsLEntry), lsCmp);
            for (int i = 0; i < count; i++)
            {
                char *pathToFile = lsEntries[i].path;

                if (isDir(pathToFile))
                {

                    blue();
                    bold();
                }
                else if (isExecutable(pathToFile))
                {

                    green();
                    bold();
                }

                printf("%s\n", lsEntries[i].name);
                reset();
            }

            closedir(dir);

            for (int i = 0; i < count; i++)
            {
                free(lsEntries[i].name);
                free(lsEntries[i].path);
            }
            free(lsEntries);
            if (i != dirCount - 1)
                printf("\n");
        }
    }
    else
    {
        struct lsLEntry *lsEntriesF = (struct lsLEntry *)malloc(sizeof(struct lsLEntry) * fileCount);
        for (int index = 0; index < fileCount; index++)
        {
            struct stat path_stat;
            stat(files[index], &path_stat);

            char fileType;
            if (S_ISREG(path_stat.st_mode))
            {
                fileType = '-';
            }
            else if (S_ISDIR(path_stat.st_mode))
            {
                fileType = 'd';
            }
            else if (S_ISCHR(path_stat.st_mode))
            {
                fileType = 'c';
            }
            else if (S_ISBLK(path_stat.st_mode))
            {
                fileType = 'b';
            }
            else if (S_ISFIFO(path_stat.st_mode))
            {
                fileType = 'p';
            }
            else if (S_ISLNK(path_stat.st_mode))
            {
                fileType = 'l';
            }
            else if (S_ISSOCK(path_stat.st_mode))
            {
                fileType = 's';
            }
            else
            {
                fileType = 'n';
            }

            // printf("%c", fileType);
            lsEntriesF[index].fileType = fileType;

            char permissions[10];
            // check for sticky and setgid bit
            permissions[0] = (path_stat.st_mode & S_IRUSR) ? 'r' : '-';
            permissions[1] = (path_stat.st_mode & S_IWUSR) ? 'w' : '-';
            if (path_stat.st_mode & S_ISUID)
            {
                permissions[2] = (path_stat.st_mode & S_IXUSR) ? 's' : 'S';
            }
            else
            {
                permissions[2] = (path_stat.st_mode & S_IXUSR) ? 'x' : '-';
            }
            permissions[3] = (path_stat.st_mode & S_IRGRP) ? 'r' : '-';
            permissions[4] = (path_stat.st_mode & S_IWGRP) ? 'w' : '-';
            if (path_stat.st_mode & S_ISGID)
            {
                permissions[5] = (path_stat.st_mode & S_IXGRP) ? 's' : 'S';
            }
            else
            {
                permissions[5] = (path_stat.st_mode & S_IXGRP) ? 'x' : '-';
            }
            permissions[6] = (path_stat.st_mode & S_IROTH) ? 'r' : '-';
            permissions[7] = (path_stat.st_mode & S_IWOTH) ? 'w' : '-';
            if (path_stat.st_mode & S_ISVTX)
            {
                permissions[8] = (path_stat.st_mode & S_IXOTH) ? 't' : 'T';
            }
            else
            {
                permissions[8] = (path_stat.st_mode & S_IXOTH) ? 'x' : '-';
            }
            permissions[9] = '\0';

            // printf("%s ", permissions);
            strcpy(lsEntriesF[index].permissions, permissions);

            // printf("%ld ", path_stat.st_nlink);
            lsEntriesF[index].nlink = path_stat.st_nlink;

            struct passwd *pw = getpwuid(path_stat.st_uid);
            struct group *gr = getgrgid(path_stat.st_gid);

            // printf("%s %s ", pw->pw_name, gr->gr_name);
            lsEntriesF[index].owner = (char *)malloc(strlen(pw->pw_name) + 1);
            strcpy(lsEntriesF[index].owner, pw->pw_name);
            lsEntriesF[index].group = (char *)malloc(strlen(gr->gr_name) + 1);
            strcpy(lsEntriesF[index].group, gr->gr_name);

            // printf("%ld ", path_stat.st_size);
            lsEntriesF[index].size = path_stat.st_size;

            char *time = ctime(&path_stat.st_mtime);

            // printf("Working here");
            // fflush(stdout);
            lsEntriesF[index].time = (char *)malloc(strlen(time) + 1);
            strcpy(lsEntriesF[index].time, time);
            // remove newline
            lsEntriesF[index].time[strlen(time) - 1] = '\0';

            // printf("%s\n", entry->d_name);
            lsEntriesF[index].name = (char *)malloc(strlen(files[index]) + 1);
            // printf("Working here");
            // fflush(stdout);
            strcpy(lsEntriesF[index].name, files[index]);
            lsEntriesF[index].path = (char *)malloc(strlen(files[index]) + 1);
            strcpy(lsEntriesF[index].path, files[index]);
        }
        int maxLinksF = 0;
        int maxOwnerF = 0;
        int maxGroupF = 0;
        int maxSizeF = 0;
        int maxDateF = 0;
        int maxNameF = 0;

        for (int i = 0; i < fileCount; i++)
        {
            if (maxLinksF < lsEntriesF[i].nlink)
            {
                maxLinksF = lsEntriesF[i].nlink;
            }
            if (maxOwnerF < strlen(lsEntriesF[i].owner))
            {
                maxOwnerF = strlen(lsEntriesF[i].owner);
            }
            if (maxGroupF < strlen(lsEntriesF[i].group))
            {
                maxGroupF = strlen(lsEntriesF[i].group);
            }
            if (maxSizeF < lsEntriesF[i].size)
            {
                maxSizeF = lsEntriesF[i].size;
            }
            if (maxDateF < strlen(lsEntriesF[i].time))
            {
                maxDateF = strlen(lsEntriesF[i].time);
            }
            if (maxNameF < strlen(lsEntriesF[i].name))
            {
                maxNameF = strlen(lsEntriesF[i].name);
            }
        }
        maxLinksF = getDigits(maxLinksF);
        maxSizeF = getDigits(maxSizeF);

        qsort(lsEntriesF, fileCount, sizeof(struct lsLEntry), lsCmp);

        for (int i = 0; i < fileCount; i++)
        {
            printf("%c%s %*d %-*s %-*s %*d %s ", lsEntriesF[i].fileType, lsEntriesF[i].permissions, maxLinksF, lsEntriesF[i].nlink, maxOwnerF, lsEntriesF[i].owner, maxGroupF, lsEntriesF[i].group, maxSizeF, lsEntriesF[i].size, lsEntriesF[i].time);

            if (isDir(lsEntriesF[i].path))
            {
                blue();
                bold();
            }
            else if (isExecutable(lsEntriesF[i].path))
            {
                green();
                bold();
            }

            printf("%s\n", lsEntriesF[i].name);
            reset();
        }

        for (int i = 0; i < fileCount; i++)
        {
            free(lsEntriesF[i].owner);
            free(lsEntriesF[i].group);
            free(lsEntriesF[i].time);
            free(lsEntriesF[i].name);
            free(lsEntriesF[i].path);
        }

        free(lsEntriesF);
        if (dirCount && fileCount)
            printf("\n");

        for (int i = 0; i < dirCount; i++)
        {
            unsigned long long int total = 0;
            if (dirCount != 1 || fileCount)
                printf("%s:\n", dirs[i]);

            DIR *dir = opendir(dirs[i]);
            struct dirent *entry;
            // get number of files
            int count = 0;
            while ((entry = readdir(dir)) != NULL)
            {
                if (a == 0 && entry->d_name[0] == '.')
                {
                    continue;
                }
                count++;
            }
            closedir(dir);
            dir = opendir(dirs[i]);

            struct lsLEntry *lsEntries = (struct lsLEntry *)malloc(sizeof(struct lsLEntry) * count);
            int index = 0;

            while ((entry = readdir(dir)) != NULL)
            {
                if (a == 0 && entry->d_name[0] == '.')
                {
                    continue;
                }
                struct stat path_stat;
                char *path = (char *)malloc(MAX_BUF);
                strcpy(path, dirs[i]);
                // printf("dir here: %s\n", path);s
                strcat(path, "/");
                // printf("dir here: %s\n", path);
                strcat(path, entry->d_name);
                // printf("dir here: %s\n", path);
                lsEntries[index].path = (char *)malloc(strlen(path) + 1);
                strcpy(lsEntries[index].path, path);
                stat(path, &path_stat);
                total += path_stat.st_blocks;

                free(path);

                char fileType;
                if (S_ISREG(path_stat.st_mode))
                {
                    fileType = '-';
                }
                else if (S_ISDIR(path_stat.st_mode))
                {
                    fileType = 'd';
                }
                else if (S_ISCHR(path_stat.st_mode))
                {
                    fileType = 'c';
                }
                else if (S_ISBLK(path_stat.st_mode))
                {
                    fileType = 'b';
                }
                else if (S_ISFIFO(path_stat.st_mode))
                {
                    fileType = 'p';
                }
                else if (S_ISLNK(path_stat.st_mode))
                {
                    fileType = 'l';
                }
                else if (S_ISSOCK(path_stat.st_mode))
                {
                    fileType = 's';
                }
                else
                {
                    fileType = 'n';
                }

                // printf("%c", fileType);
                lsEntries[index].fileType = fileType;

                char permissions[10];
                // check for sticky and setgid bit
                permissions[0] = (path_stat.st_mode & S_IRUSR) ? 'r' : '-';
                permissions[1] = (path_stat.st_mode & S_IWUSR) ? 'w' : '-';
                if (path_stat.st_mode & S_ISUID)
                {
                    permissions[2] = (path_stat.st_mode & S_IXUSR) ? 's' : 'S';
                }
                else
                {
                    permissions[2] = (path_stat.st_mode & S_IXUSR) ? 'x' : '-';
                }
                permissions[3] = (path_stat.st_mode & S_IRGRP) ? 'r' : '-';
                permissions[4] = (path_stat.st_mode & S_IWGRP) ? 'w' : '-';
                if (path_stat.st_mode & S_ISGID)
                {
                    permissions[5] = (path_stat.st_mode & S_IXGRP) ? 's' : 'S';
                }
                else
                {
                    permissions[5] = (path_stat.st_mode & S_IXGRP) ? 'x' : '-';
                }
                permissions[6] = (path_stat.st_mode & S_IROTH) ? 'r' : '-';
                permissions[7] = (path_stat.st_mode & S_IWOTH) ? 'w' : '-';
                if (path_stat.st_mode & S_ISVTX)
                {
                    permissions[8] = (path_stat.st_mode & S_IXOTH) ? 't' : 'T';
                }
                else
                {
                    permissions[8] = (path_stat.st_mode & S_IXOTH) ? 'x' : '-';
                }
                permissions[9] = '\0';

                // printf("%s ", permissions);
                strcpy(lsEntries[index].permissions, permissions);

                // printf("%ld ", path_stat.st_nlink);
                lsEntries[index].nlink = path_stat.st_nlink;

                struct passwd *pw = getpwuid(path_stat.st_uid);
                struct group *gr = getgrgid(path_stat.st_gid);

                // printf("%s %s ", pw->pw_name, gr->gr_name);
                lsEntries[index].owner = (char *)malloc(strlen(pw->pw_name) + 1);
                strcpy(lsEntries[index].owner, pw->pw_name);
                lsEntries[index].group = (char *)malloc(strlen(gr->gr_name) + 1);
                strcpy(lsEntries[index].group, gr->gr_name);

                // printf("%ld ", path_stat.st_size);
                lsEntries[index].size = path_stat.st_size;

                char *time = ctime(&path_stat.st_mtime);

                // printf("Working here");
                // fflush(stdout);
                lsEntries[index].time = (char *)malloc(strlen(time) + 1);
                strcpy(lsEntries[index].time, time);
                // remove newline
                lsEntries[index].time[strlen(time) - 1] = '\0';

                // printf("%s\n", entry->d_name);
                lsEntries[index].name = (char *)malloc(strlen(entry->d_name) + 1);
                // printf("Working here");
                // fflush(stdout);
                strcpy(lsEntries[index].name, entry->d_name);

                index++;
            }
            printf("total %lld\n", total / 2);

            // find max length of each field
            int maxLinks = 0;
            int maxOwner = 0;
            int maxGroup = 0;
            int maxSize = 0;
            int maxDate = 0;
            int maxName = 0;

            for (int i = 0; i < count; i++)
            {
                // print all values
                // printf("fileType: %c\n", lsEntries[i].fileType);
                // printf("permissions: %s\n", lsEntries[i].permissions);
                // printf("nlink: %d\n", lsEntries[i].nlink);
                // printf("owner: %s\n", lsEntries[i].owner);
                // printf("group: %s\n", lsEntries[i].group);
                // printf("size: %d\n", lsEntries[i].size);
                // printf("time: %s\n", lsEntries[i].time);
                // printf("name: %s\n", lsEntries[i].name);
                // printf("path: %s\n", lsEntries[i].path);
                // printf("\n");
                // fflush(stdout);
                if (maxLinks < lsEntries[i].nlink)
                {
                    maxLinks = lsEntries[i].nlink;
                }
                if (maxOwner < strlen(lsEntries[i].owner))
                {
                    maxOwner = strlen(lsEntries[i].owner);
                }
                if (maxGroup < strlen(lsEntries[i].group))
                {
                    maxGroup = strlen(lsEntries[i].group);
                }
                if (maxSize < lsEntries[i].size)
                {
                    maxSize = lsEntries[i].size;
                }
                if (maxDate < strlen(lsEntries[i].time))
                {
                    maxDate = strlen(lsEntries[i].time);
                }
                if (maxName < strlen(lsEntries[i].name))
                {
                    maxName = strlen(lsEntries[i].name);
                }
            }
            maxLinks = getDigits(maxLinks);
            maxSize = getDigits(maxSize);
            qsort(lsEntries, count, sizeof(struct lsLEntry), lsCmp);
            for (int i = 0; i < count; i++)
            {
                printf("%c%s %*d %-*s %-*s %*d %s ", lsEntries[i].fileType, lsEntries[i].permissions, maxLinks, lsEntries[i].nlink, maxOwner, lsEntries[i].owner, maxGroup, lsEntries[i].group, maxSize, lsEntries[i].size, lsEntries[i].time);

                if (isDir(lsEntries[i].path))
                {
                    blue();
                    bold();
                }
                else if (isExecutable(lsEntries[i].path))
                {

                    green();
                    bold();
                }

                printf("%s\n", lsEntries[i].name);
                reset();
            }

            closedir(dir);
            if (i != dirCount - 1)
                printf("\n");

            for (int i = 0; i < count; i++)
            {
                free(lsEntries[i].owner);
                free(lsEntries[i].group);
                free(lsEntries[i].time);
                free(lsEntries[i].name);
                free(lsEntries[i].path);
            }

            free(lsEntries);
        }
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

char **getFileList(char *dir)
{
    // set root if dir empty
    char *root = "/";
    DIR *d;
    struct dirent *dirEntry;
    if (strlen(dir) == 0)
    {
        d = opendir(root);
    }
    else
    {
        d = opendir(dir);
    }

    if (d)
    {
        int count = 0;
        while ((dirEntry = readdir(d)) != NULL)
        {
            count++;
        }
        closedir(d);
        char **files = (char **)malloc((count + 1) * sizeof(char *));
        d = opendir(dir);
        int index = 0;
        while ((dirEntry = readdir(d)) != NULL)
        {
            files[index] = (char *)malloc(strlen(dirEntry->d_name) + 1);
            strcpy(files[index], dirEntry->d_name);
            index++;
        }
        // add null terminator
        files[index] = NULL;
        closedir(d);
        return files;
    }
    else
    {
        return NULL;
    }
}

void jobs(char *args[], int argc)
{
    int showRunning = 0;
    int showStopped = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(args[i], "-r") == 0)
        {
            showRunning = 1;
        }
        else if (strcmp(args[i], "-s") == 0)
        {
            showStopped = 1;
        }
        else if (strcmp(args[i], "-rs") == 0 || strcmp(args[i], "-sr") == 0)
        {
            showRunning = 1;
            showStopped = 1;
        }
        else
        {
            printf("Invalid argument %s\n", args[i]);
            return;
        }
    }

    for (int i = 0; i < curbackgroundJobs; i++)
    {
        // get stat file
        char statPath[100];
        sprintf(statPath, "/proc/%d/stat", backgroundJobs[i].pid);
        FILE *stat = fopen(statPath, "r");
        if (stat == NULL || kill(backgroundJobs[i].pid, 0) == -1)
        {
            continue;
        }
        char state;
        fscanf(stat, "%*d %*s %c", &state);
        fclose(stat);

        if (showRunning && state == 'T' || showStopped && state != 'T')
        {
            continue;
        }

        char running[10];
        if (state == 'T')
        {
            strcpy(running, "Stopped");
        }
        else
        {
            strcpy(running, "Running");
        }

        printf("[%d] %s %s [%d]\n", i + 1, running, backgroundJobs[i].cmd, backgroundJobs[i].pid);
    }
}

void sendSignal(char *args[], int argc)
{
    if (argc != 3)
    {
        printf("Invalid number of arguments.\nArguments must be exactly 3 in the format sig <job-number> <signal>.\n");
        return;
    }

    int jobNumber = atoi(args[1]);
    int signal = atoi(args[2]);

    if (jobNumber > curbackgroundJobs || jobNumber < 1)
    {
        printf("Invalid job number.\n");
        return;
    }

    if (kill(backgroundJobs[jobNumber - 1].pid, signal) == -1)
    {
        printf("Error sending signal.\n");
    }
}

void resumeBG(char *args[], int argc)
{
    if (argc != 2)
    {
        printf("Invalid number of arguments.\nArguments must be exactly 2 in the format bg <job-number>.\n");
        return;
    }

    int jobNumber = atoi(args[1]);

    if (jobNumber > curbackgroundJobs || jobNumber < 1)
    {
        printf("Invalid job number.\n");
        return;
    }

    if (kill(backgroundJobs[jobNumber - 1].pid, SIGCONT) == -1)
    {
        printf("Error sending signal.\n");
    }
}

void bringFG(char *args[], int argc)
{
    if (argc != 2)
    {
        printf("Invalid number of arguments.\nArguments must be exactly 2 in the format fg <job-number>.\n");
        return;
    }

    int jobNumber = atoi(args[1]);

    if (jobNumber > curbackgroundJobs || jobNumber < 1)
    {
        printf("Invalid job number.\n");
        return;
    }

    int jobIdx = jobNumber - 1;
    // bring to foreground

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, getpgid(backgroundJobs[jobIdx].pid));
    tcsetpgrp(STDOUT_FILENO, getpgid(backgroundJobs[jobIdx].pid));

    int status;

    if (kill(backgroundJobs[jobIdx].pid, SIGCONT) == -1)
    {
        printf("Error sending signal.\n");
    }
    else
    {
        waitpid(backgroundJobs[jobIdx].pid, &status, WUNTRACED);
    }
    tcsetpgrp(STDIN_FILENO, getpgrp());
    tcsetpgrp(STDOUT_FILENO, getpgrp());

    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    // printf("Fg over\n");
    // check status
    if (WIFEXITED(status) || WIFSIGNALED(status))
    {

        for (int i = jobIdx; i < curbackgroundJobs - 1; i++)
        {
            backgroundJobs[i] = backgroundJobs[i + 1];
        }
        curbackgroundJobs--;
    }
}