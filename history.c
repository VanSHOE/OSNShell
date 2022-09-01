#include "history.h"
#include "globalData.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void readHistory()
{
    curHistHead = -1;
    curHistTail = -1;
    for (int i = 0; i < historyLen; i++)
    {
        cmdHistory[i] = (char *)malloc(MAX_BUF);
    }

    int fd = open("history.txt", O_RDONLY);
    if (fd == -1)
    {
        return;
    }
    char *line = (char *)malloc(30 * MAX_BUF);
    int len = read(fd, line, 30 * MAX_BUF);
    line[len] = '\0';
    char *token = strtok(line, ";");
    while (token != NULL)
    {
        curHistHead++;
        curHistTail = 0;
        strcpy(cmdHistory[curHistHead], token);
        token = strtok(NULL, ";");
    }
    close(fd);
}

void writeHistory()
{
    int fd = open("history.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int curPtr = curHistTail;
    // printf("curHistHead: %d, curHistTail: %d", curHistHead, curHistTail);
    // // clear stdio buffer
    // fflush(stdout);
    while (1)
    {
        write(fd, cmdHistory[curPtr], strlen(cmdHistory[curPtr]));
        write(fd, ";", 1);

        if (curPtr == curHistHead)
        {
            break;
        }
        curPtr = (curPtr + 1) % historyLen;
    }

    close(fd);
}

void addtoMem(char *cmd[], int argc)
{
    char temp[MAX_BUF];
    strcpy(temp, cmd[0]);
    for (int i = 1; i < argc; i++)
    {
        strcat(temp, " ");
        strcat(temp, cmd[i]);
    }
    // printf("|%s|\n", temp);
    if (curHistHead != -1 && strcmp(cmdHistory[curHistHead], temp) == 0)
    {
        return;
    }

    if ((curHistHead + 1) % historyLen == curHistTail || curHistTail == -1)
    {
        curHistTail = (curHistTail + 1) % historyLen;
    }

    curHistHead = (curHistHead + 1) % historyLen;

    strcpy(cmdHistory[curHistHead], temp);

    writeHistory();
}

void printHistory()
{
    if (curHistHead == -1 || curHistTail == -1)
    {
        return;
    }

    int curPtr = curHistTail;
    while (1)
    {
        printf("%s\n", cmdHistory[curPtr]);

        if (curPtr == curHistHead)
        {
            break;
        }
        curPtr = (curPtr + 1) % historyLen;
    }
}