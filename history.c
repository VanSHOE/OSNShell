#include "history.h"
#include "globalData.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

void readHistory()
{
    for (int i = 0; i < historyLen; i++)
    {
        cmdHistory[i] = (char *)malloc(MAX_BUF);
    }

    int fd = open("history.txt", O_RDONLY);
    if (fd == -1)
    {
        return;
    }
    char *line = (char *)malloc(MAX_BUF);
    int i = 0;
    curHistHead = -1;
    while (read(fd, line, MAX_BUF) > 0)
    {
        strcpy(cmdHistory[i], line);
        curHistHead = i++;
    }
    close(fd);
}

void writeHistory()
{
    int fd = open("history.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    for (int i = 0; i < historyLen; i++)
    {
        write(fd, cmdHistory[i], strlen(cmdHistory[i]));
        write(fd, "\n", 1);
    }

    close(fd);
}

void addtoMem(char *cmd)
{
    // return if equal
    if (curHistHead == -1 || strcmp(cmdHistory[curHistHead], cmd) == 0)
    {
        return;
    }

    curHistHead = (curHistHead + 1) % historyLen;
    strcpy(cmdHistory[curHistHead], cmd);
    writeHistory();
}