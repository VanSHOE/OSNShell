#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "colors.h"
#include "history.h"
#include "builtin.h"
#include "delegateCommands.h"
#include "globalData.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>

void stripQuotes(char *str)
{
    // printf("\nBefore: %s\n", str);
    int newTermIndex = strlen(str) - 2;

    if (newTermIndex < 0)
        return;

    // strip quotes single and double
    if ((str[0] == '\'' && str[strlen(str) - 1] == '\'') || (str[0] == '\"' && str[strlen(str) - 1] == '\"'))
    {
        for (int i = 0; i < strlen(str) - 1; i++)
        {
            str[i] = str[i + 1];
        }
        str[newTermIndex] = '\0';
    }

    // printf("After: %s\n", str);
}

char *commonPrefix(char *str1, char *str2)
{
    // find the longest prefix common to both strings
    int i = 0;
    while (str1[i] && str2[i] && str1[i] == str2[i])
        i++;
    char *prefix = (char *)malloc(sizeof(char) * (i + 1));
    strncpy(prefix, str1, i);
    prefix[i] = '\0';
    return prefix;
}

int timeCorrect = 0;

void printPrompt()
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
    if (lastTime >= 1 && timeCorrect)
    {
        printf("took %.17gs", (double)lastTime);
        timeCorrect = 0;
    }
    printf("> ");
}
void die(const char *s)
{
    perror(s);
    exit(1);
}

struct termios orig_termios;

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

char *showPrompt()
{
    char *inp = malloc(sizeof(char) * MAX_BUF);
    // printPrompt();
    // fgets(inp, MAX_BUF, stdin);
    // return inp;
    char c;
    while (1)
    {
        setbuf(stdout, NULL);
        enableRawMode();
        printPrompt();
        memset(inp, '\0', MAX_BUF);
        int pt = 0;
        while (read(STDIN_FILENO, &c, 1) == 1)
        {
            if (iscntrl(c))
            {
                if (c == 10)
                    break;
                // else if (c == 27)
                // {
                //     char buf[3];
                //     buf[2] = 0;
                //     if (read(STDIN_FILENO, buf, 2) == 2)
                //     { // length of escape code
                //       printf("\rarrow key: %s", buf);
                //     }
                // }
                else if (c == 127)
                { // backspace
                    if (pt > 0)
                    {
                        if (inp[pt - 1] == 9)
                        {
                            for (int i = 0; i < 7; i++)
                            {
                                printf("\b");
                            }
                        }
                        inp[--pt] = '\0';
                        printf("\b \b");
                    }
                }
                else if (c == 9)
                { // TAB character
                    // continue;
                    // search for last space else 0
                    int prefStart = 0;
                    for (int i = 0; i < pt; i++)
                    {
                        if (inp[i] == ' ')
                            prefStart = i + 1;
                    }

                    char *curPrefix = (char *)malloc(sizeof(char) * (strlen(inp) - prefStart + 1));
                    strcpy(curPrefix, inp + prefStart);

                    // check curPrefix is a dir
                    int lastSlash = -1;
                    for (int i = 0; i < strlen(curPrefix); i++)
                    {
                        if (curPrefix[i] == '/')
                            lastSlash = i;
                    }

                    char *lastPath = (char *)malloc(sizeof(char) * (lastSlash + 5));
                    if (lastSlash == -1)
                    {
                        strcpy(lastPath, ".");
                    }
                    else
                    {
                        strncpy(lastPath, curPrefix, lastSlash + 1);
                        lastPath[lastSlash] = '/';
                        lastPath[lastSlash + 1] = '\0';
                    }

                    // printf("\nLast path: %s:%d\n", lastPath, strlen(lastPath));
                    // continue;
                    struct stat st;
                    char **fileList = NULL;
                    char *rectifiedLastPath = parsePathforHome(lastPath);

                    // printf("\nRectified path: %s\n", rectifiedLastPath);

                    if (stat(rectifiedLastPath, &st) == 0 && S_ISDIR(st.st_mode))
                    {
                        fileList = getFileList(rectifiedLastPath);
                        // printf("\nGetting filelist at: %s with last slash at: %d\n", lastPath, lastSlash);
                        prefStart += lastSlash + 1;

                        strcpy(curPrefix, inp + prefStart);
                        // printf("Latest prefix: %s\n", curPrefix);
                    }
                    else
                    {
                        fileList = getFileList(".");

                        strcpy(rectifiedLastPath, ".");
                    }

                    // char **fileList = getFileList(".");
                    if (fileList == NULL)
                    {
                        continue;
                    }

                    int i = 0;

                    char **filteredList = (char **)malloc(sizeof(char *) * MAX_BUF);
                    int filteredListSize = 0;
                    while (fileList[i] != NULL)
                    {
                        if (strstr(fileList[i], curPrefix) == fileList[i])
                        {
                            filteredList[filteredListSize++] = fileList[i];
                        }
                        i++;
                    }
                    // printf("Count: %d\n", i);

                    if (!filteredListSize)
                    {
                        i = 0;
                        while (fileList[i] != NULL)
                        {
                            free(fileList[i]);
                            i++;
                        }
                        free(fileList);
                        free(curPrefix);
                        free(lastPath);
                        free(filteredList);
                        continue;
                    }
                    else if (filteredListSize == 1)
                    {
                        // only one file, directly put it in input
                        for (int i = prefStart; i < pt; i++)
                        {
                            printf("\b \b");
                        }
                        for (int i = prefStart; i < pt; i++)
                        {
                            inp[i] = '\0';
                        }
                        pt = prefStart;
                        for (int i = 0; i < strlen(filteredList[0]); i++)
                        {
                            inp[pt++] = filteredList[0][i];
                            printf("%c", filteredList[0][i]);
                        }

                        // check if its a file or dir
                        struct stat st;
                        char *path = (char *)malloc(sizeof(char) * (strlen(filteredList[0]) + strlen(rectifiedLastPath) + 2));
                        strcpy(path, rectifiedLastPath);
                        free(lastPath);
                        strcat(path, "/");
                        strcat(path, filteredList[0]);
                        // printf("\nChecking path: %s\n", path);

                        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
                        {
                            inp[pt++] = '/';
                            printf("/");
                        }
                        else
                        {
                            inp[pt++] = ' ';
                            printf(" ");
                        }
                        free(path);
                        i = 0;
                        while (fileList[i] != NULL)
                        {
                            free(fileList[i]);
                            i++;
                        }
                        free(fileList);
                        free(curPrefix);
                        free(filteredList);
                        continue;
                    }
                    free(lastPath);
                    // print filtered list
                    printf("\n");
                    for (int i = 0; i < filteredListSize; i++)
                    {
                        printf("%s\n", filteredList[i]);
                    }
                    printPrompt();
                    // printf("New Prompt");
                    for (int i = 0; i < pt; i++)
                    {
                        printf("%c", inp[i]);
                    }

                    char *prefix = (char *)malloc(sizeof(char) * strlen(filteredList[0]));
                    strcpy(prefix, filteredList[0]);
                    for (int i = 1; i < filteredListSize; i++)
                    {
                        char *newPrefix = commonPrefix(prefix, filteredList[i]);
                        free(prefix);
                        prefix = newPrefix;
                    }
                    // printf("This ran");
                    // fflush(stdout);
                    if (strlen(prefix) > strlen(curPrefix))
                    {
                        for (int i = prefStart; i < pt; i++)
                        {
                            printf("\b \b");
                        }
                        for (int i = prefStart; i < pt; i++)
                        {
                            inp[i] = '\0';
                        }
                        pt = prefStart;
                        for (int i = 0; i < strlen(prefix); i++)
                        {
                            inp[pt++] = prefix[i];
                            printf("%c", prefix[i]);
                        }
                    }
                    free(prefix);
                    // printf("This ran\n");
                    fflush(stdout);
                    i = 0;
                    while (fileList[i] != NULL)
                    {
                        // printf("This ran: %s at addr: %d\n", fileList[i], (int)fileList[i]);
                        fflush(stdout);
                        free(fileList[i]); // WHY IS THIS CAUSING A CRASH????????????
                        i++;
                    }
                    // printf("This ran2\n");
                    fflush(stdout);
                    free(fileList);
                    free(curPrefix);
                    free(filteredList);
                }
                else if (c == 4)
                {
                    exit(0);
                }
                else
                {
                    printf("%d\n", c);
                }
            }
            else
            {
                inp[pt++] = c;
                printf("%c", c);
            }
        }
        disableRawMode();

        // printf("\nInput Read: [%s]\n", inp);
        printf("\n");
        return inp;
    }
}

void childDead()
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0)
    {
        int i;
        for (i = 0; i < curbackgroundJobs; i++)
        {
            if (backgroundJobs[i].pid == pid)
            {
                if (WEXITSTATUS(status) == 255)
                {
                    printf("\n%s: command not found\n", backgroundJobs[i].name);
                    printPrompt();
                    fflush(stdout);
                }
                else
                {
                    printf("\n%s with pid %d exited ", backgroundJobs[i].name, pid);
                    if (WIFEXITED(status))
                    {

                        if (WEXITSTATUS(status) == 0)
                        {
                            printf("normally.\n");
                        }
                        else
                        {
                            printf("abnormally with status = %d\n", WEXITSTATUS(status));
                        }

                        printPrompt();
                        fflush(stdout);
                    }
                    else if (WIFSIGNALED(status))
                    {
                        printf("abnormally with signal = %d\n", WTERMSIG(status));
                        fflush(stdout);
                        printPrompt();
                        fflush(stdout);
                    }
                    else if (WIFSTOPPED(status))
                    {
                        printf("abnormally with signal = %d\n", WSTOPSIG(status));
                        fflush(stdout);
                        printPrompt();
                        fflush(stdout);
                    }
                    else
                    {
                        printf("abnormally\n");
                        fflush(stdout);
                        printPrompt();
                        fflush(stdout);
                    }
                }

                // free
                if (backgroundJobs[i].name != NULL)
                {
                    free(backgroundJobs[i].name);
                }
                if (backgroundJobs[i].cmd != NULL)
                {
                    free(backgroundJobs[i].cmd);
                }

                for (int j = i; j < curbackgroundJobs; j++)
                {
                    backgroundJobs[j] = backgroundJobs[j + 1];
                }
                curbackgroundJobs--;
                break;
            }
        }
    }
    // clear output buffer
}

void dontExit()
{
    for (int i = 0; i < curforegroundJobs; i++)
    {
        kill(foregroundJobs[i].pid, SIGINT);
        free(foregroundJobs[i].name);
        free(foregroundJobs[i].cmd);
    }
    curforegroundJobs = 0;
    printf("\n");
    printPrompt();
    fflush(stdout);
}

void bgTheFg()
{
    printf("\n");
    printPrompt();
    fflush(stdout);
}

int main(void)
{
    struct sigaction ctrlC;

    ctrlC.sa_handler = dontExit;
    sigemptyset(&ctrlC.sa_mask);
    ctrlC.sa_flags = SA_RESTART;

    sigaction(SIGINT, &ctrlC, NULL);

    struct sigaction ctrlZ;

    ctrlZ.sa_handler = bgTheFg;
    sigemptyset(&ctrlZ.sa_mask);
    ctrlZ.sa_flags = SA_RESTART;

    sigaction(SIGTSTP, &ctrlZ, NULL);

    struct sigaction child;
    child.sa_handler = childDead;
    sigemptyset(&child.sa_mask);
    child.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &child, NULL);

    exitFlag = 0;
    curbackgroundJobs = 0;
    // set current path as shell home malloc
    shellHome = (char *)malloc(MAX_BUF);
    getcwd(shellHome, MAX_BUF);

    readHistory();
    OLDPWD = NULL;
    // backout stdout and stdin using dup2
    int stdinCopy = dup(STDIN_FILENO);
    int stdoutCopy = dup(STDOUT_FILENO);

    while (!exitFlag)
    {
        // printf("no");
        curforegroundJobs = 0;
        char *in = showPrompt();

        // printf("Input before copy: %s\n", in);
        // printf("in is: %s", in);
        lastTime = 0;
        addtoMemDirect(in);
        // add ; after every &

        char *inCopy = (char *)malloc(2000);
        int j = 0;
        for (int i = 0; i < strlen(in); i++)
        {
            if (in[i] == '&')
            {
                inCopy[j++] = ' ';
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
        // printf("Input after copy: %s\n", in);
        free(inCopy);

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
            // reset stdin and stdout
            dup2(stdinCopy, STDIN_FILENO);
            dup2(stdoutCopy, STDOUT_FILENO);

            int pipedCommands = 0;
            char *combinedCommand = (char *)malloc(strlen(cmdArray[i]) + 1);
            strcpy(combinedCommand, cmdArray[i]);
            char *combinedBackup = (char *)malloc(strlen(cmdArray[i]) + 1);
            strcpy(combinedBackup, cmdArray[i]);

            char *pipedCommand = strtok(combinedCommand, "|");
            while (pipedCommand != NULL)
            {
                pipedCommands++;
                pipedCommand = strtok(NULL, "|");
            }
            strcpy(combinedCommand, combinedBackup);

            char **pipedCommandArray = (char **)malloc(sizeof(char *) * pipedCommands);
            pipedCommandArray[0] = strtok(combinedCommand, "|");
            for (int j = 1; j < pipedCommands; j++)
            {
                pipedCommandArray[j] = strtok(NULL, "|");
            }

            int pipesRequired = pipedCommands - 1;
            int **pipefd = (int **)malloc(sizeof(int *) * pipesRequired);

            for (int j = 0; j < pipesRequired; j++)
            {
                pipefd[j] = (int *)malloc(sizeof(int) * 2);
                pipe(pipefd[j]);
            }
            int *pids = (int *)malloc(sizeof(int) * pipedCommands);

            for (int ii = 0; ii < pipedCommands; ii++)
            {
                dup2(stdinCopy, STDIN_FILENO);
                dup2(stdoutCopy, STDOUT_FILENO);
                // printf("Command: %s\n", pipedCommandArray[ii]);
                timeCorrect = 0;
                char *cmd = pipedCommandArray[ii];
                // printf("%s\n", cmd);
                // copy
                char *cmdCopy = (char *)malloc(strlen(cmd) + 1);
                strcpy(cmdCopy, cmd);
                // check for input redir
                int inputIndex = -1;
                char *inputRedir = strstr(cmdCopy, "<");
                if (inputRedir != NULL)
                {
                    inputIndex = inputRedir - cmdCopy;
                    // remove it
                    cmd[inputIndex] = ' ';
                    cmdCopy[inputIndex] = ' ';
                }
                // printf("Formatted command: %s\n", cmd);
                // get input file
                char *inputFile = NULL;
                int inputRedirFlag = 0;
                if (inputIndex != -1)
                {
                    int skippedFirstSpaces = 0;
                    // inputFile = (char *)malloc(MAX_BUF);
                    int j = 0;
                    for (int k = inputIndex + 1; k < strlen(cmdCopy); k++)
                    {
                        if ((cmdCopy[k] == ' ' || cmdCopy[k] == '\t') && !skippedFirstSpaces)
                        {
                            continue;
                        }
                        else
                        {
                            skippedFirstSpaces = 1;
                            if (cmdCopy[k] == ' ')
                            {
                                break;
                            }
                            else
                            {

                                j++;
                            }
                        }
                    }
                    inputFile = (char *)malloc(j + 1);
                    skippedFirstSpaces = 0;
                    j = 0;
                    for (int k = inputIndex + 1; k < strlen(cmdCopy); k++)
                    {
                        if ((cmdCopy[k] == ' ' || cmdCopy[k] == '\t') && !skippedFirstSpaces)
                        {
                            continue;
                        }
                        else
                        {
                            skippedFirstSpaces = 1;
                            if (cmdCopy[k] == ' ')
                            {
                                break;
                            }
                            else
                            {
                                inputFile[j++] = cmdCopy[k];
                                // remove name from cmd
                                cmd[k] = ' ';
                                cmdCopy[k] = ' ';
                            }
                        }
                    }
                    inputFile[j] = '\0';
                    // check if file exists
                    FILE *fp = fopen(inputFile, "r");
                    if (fp == NULL)
                    {
                        printf("File %s does not exist\n", inputFile);
                        // printPrompt();
                        // fflush(stdout);
                        continue;
                    }
                    else
                    {
                        dup2(fileno(fp), STDIN_FILENO);
                        inputRedirFlag = 1;
                    }
                }

                // check for output redir
                int outputIndex = -1;
                int isAppend = 0;
                char *outputRedir = strstr(cmdCopy, ">");
                int outputRedirFlag = 0;

                if (outputRedir != NULL)
                {
                    outputIndex = outputRedir - cmdCopy;
                    // check for append

                    // remove it
                    cmd[outputIndex] = ' ';
                    cmdCopy[outputIndex] = ' ';
                    if (outputIndex + 1 < strlen(cmdCopy) && cmdCopy[outputIndex + 1] == '>')
                    {
                        isAppend = 1;
                        outputIndex++;
                        cmd[outputIndex] = ' ';
                        cmdCopy[outputIndex] = ' ';
                    }
                }
                // get output file
                char *outputFile = NULL;
                if (outputIndex != -1)
                {
                    int skippedFirstSpaces = 0;
                    // outputFile = (char *)malloc(MAX_BUF);
                    int j = 0;
                    for (int k = outputIndex + 1; k < strlen(cmdCopy); k++)
                    {
                        if ((cmdCopy[k] == ' ' || cmdCopy[k] == '\t') && !skippedFirstSpaces)
                        {
                            continue;
                        }
                        else
                        {
                            skippedFirstSpaces = 1;
                            if (cmdCopy[k] == ' ')
                            {
                                break;
                            }
                            else
                            {
                                j++;
                            }
                        }
                    }
                    outputFile = (char *)malloc(j + 1);
                    skippedFirstSpaces = 0;
                    j = 0;
                    for (int k = outputIndex + 1; k < strlen(cmdCopy); k++)
                    {
                        if ((cmdCopy[k] == ' ' || cmdCopy[k] == '\t') && !skippedFirstSpaces)
                        {
                            continue;
                        }
                        else
                        {
                            skippedFirstSpaces = 1;
                            if (cmdCopy[k] == ' ')
                            {
                                break;
                            }
                            else
                            {
                                outputFile[j++] = cmdCopy[k];
                                // remove name from cmd
                                cmd[k] = ' ';
                                cmdCopy[k] = ' ';
                            }
                        }
                    }
                    outputFile[j] = '\0';

                    // if file doesnt exist create using open
                    int outputFd; // = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                    if (isAppend)
                    {
                        outputFd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    }
                    else
                    {
                        outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }

                    if (outputFd == -1)
                    {
                        printf("Error opening file %s\n", outputFile);
                        printPrompt();
                        fflush(stdout);
                    }
                    else
                    {
                        dup2(outputFd, STDOUT_FILENO);
                        outputRedirFlag = 1;
                    }
                }

                // printf("Command: %s | Copy: %s\n", cmd, cmdCopy);
                // count arguments
                int args = 0;
                char *arg = strtok(cmdCopy, " \t\n");
                while (arg != NULL)
                {
                    args++;
                    arg = strtok(NULL, " \t\n");
                }

                free(cmdCopy);

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

                int procId = -2;
                if (pipedCommands != 1)
                {
                    procId = fork();

                    if (procId == 0)
                    {
                        int pipeIndex = ii - 1;
                        if (!inputRedirFlag)
                        {
                            if (pipeIndex >= 0)
                            {
                                // printf("Input ran for cmd: %s at pipeindex: %d\n", argArray[0], pipeIndex);
                                dup2(pipefd[pipeIndex][0], STDIN_FILENO);
                            }
                            else
                            {
                                dup2(stdinCopy, STDIN_FILENO);
                            }
                        }

                        if (!outputRedirFlag)
                        {
                            if (pipeIndex + 1 < pipesRequired)
                            {
                                // printf("Output ran for cmd: %s at pipeindex: %d\n", argArray[0], pipeIndex + 1);
                                dup2(pipefd[pipeIndex + 1][1], STDOUT_FILENO);
                            }
                            else
                            {
                                dup2(stdoutCopy, STDOUT_FILENO);
                            }
                        }
                        // close all
                        for (int j = 0; j < pipesRequired; j++)
                        {
                            close(pipefd[j][0]);
                            close(pipefd[j][1]);
                        }

                        int isInbuilt = callInbuilt(argArray, args);

                        if (!isInbuilt)
                        {
                            argArray[args] = NULL;
                            for (int j = 0; j < args; j++)
                            {
                                stripQuotes(argArray[j]);
                                char *temp = parsePathforHome(argArray[j]);
                                argArray[j] = (char *)malloc(strlen(temp) + 1);
                                strcpy(argArray[j], temp);
                                free(temp);
                            }
                            int res = execvp(argArray[0], argArray);
                            exit(res);
                        }

                        exit(0);
                    }
                    else
                    {
                        pids[ii] = procId;
                        foregroundJobs[curforegroundJobs].pid = procId;
                        foregroundJobs[curforegroundJobs].name = (char *)malloc(strlen(argArray[0]) + 1);
                        strcpy(foregroundJobs[curforegroundJobs].name, argArray[0]);

                        foregroundJobs[curforegroundJobs++].cmd = (char *)malloc(strlen(cmd) + 1);
                        strcpy(foregroundJobs[curforegroundJobs - 1].cmd, cmd);
                    }
                }
                else
                {
                    int isInbuilt = callInbuilt(argArray, args);

                    if (!isInbuilt)
                    {
                        for (int j = 0; j < args; j++)
                        {
                            stripQuotes(argArray[j]);
                        }
                        argArray[args] = NULL;
                        if (!strcmp(argArray[args - 1], "&"))
                        {
                            argArray[args - 1] = NULL;
                            delegate(argArray[0], argArray, 1);
                            lastTime = 0;
                        }
                        else
                        {
                            timeCorrect = 0;
                            lastTime = time(NULL);
                            delegate(argArray[0], argArray, 0);
                            lastTime = time(NULL) - lastTime;
                            timeCorrect = 1;
                        }
                    }
                }
                // printf("My Pid: %d\nCommand to run: %s\n", getpid(), argArray[0]);

                // exit(0);

                free(argArray);
                dup2(stdinCopy, STDIN_FILENO);
                dup2(stdoutCopy, STDOUT_FILENO);
            }
            // wait for all pids
            // close pipes
            for (int j = 0; j < pipesRequired; j++)
            {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }

            if (pipedCommands != 1)
                for (int j = 0; j < pipedCommands; j++)
                {
                    waitpid(pids[j], NULL, 0);
                    for (int i = 0; i < curforegroundJobs; i++)
                    {
                        if (foregroundJobs[i].pid == pids[j])
                        {
                            free(foregroundJobs[i].name);
                            free(foregroundJobs[i].cmd);
                            for (int j = i; j < curforegroundJobs - 1; j++)
                            {
                                foregroundJobs[j] = foregroundJobs[j + 1];
                            }
                            curforegroundJobs--;
                            break;
                        }
                    }
                }

            free(pids);
            free(pipefd);
            free(combinedBackup);
            free(combinedCommand);
            free(pipedCommandArray);
        }

        free(cmdArray);

        free(in);
    }

    return 0;
}