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
    int i = 0;

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
        write(fd, ";", 1);
    }

    close(fd);
}

void addtoMem(char *cmd[], int argc)
{
    // return if equal
    char temp[MAX_BUF];
    strcpy(temp, cmd[0]);
    for (int i = 1; i < argc; i++)
    {
        strcat(temp, " ");
        strcat(temp, cmd[i]);
    }
    printf("|%s|\n", temp);
    if (curHistHead != -1 && strcmp(cmdHistory[curHistHead], temp) == 0)
    {
        return;
    }

    curHistHead = (curHistHead + 1) % historyLen;

    strcpy(cmdHistory[curHistHead], temp);

    writeHistory();
}

void printHistory()
{
    int i = curHistHead;
    int j = 0;
    while (j < historyLen)
    {
        if (cmdHistory[i][0] != '\0')
        {
            printf("%d %s   ", j + 1, cmdHistory[i]);

            if (j % 2 == 1)
            {
                printf("\n");
            }

            j++;

            i = (i - 1 + historyLen) % historyLen;

            if (i == curHistHead)
            {
                break;
            }

            if (j % 2 == 1)
            {
                printf("\t");
            }
        }