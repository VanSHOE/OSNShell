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
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>

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
                    char **fileList = getFileList(".");
                    if (fileList == NULL)
                    {
                        continue;
                    }
                    // search for last space else 0
                    int lastSpace = 0;
                    for (int i = 0; i < pt; i++)
                    {
                        if (inp[i] == ' ')
                            lastSpace = i + 1;
                    }

                    int i = 0;
                    char *curPrefix = (char *)malloc(sizeof(char) * (strlen(inp) - lastSpace + 1));
                    strcpy(curPrefix, inp + lastSpace);
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
                        free(filteredList);
                        continue;
                    }
                    else if (filteredListSize == 1)
                    {
                        // only one file, directly put it in input
                        for (int i = lastSpace; i < pt; i++)
                        {
                            printf("\b \b");
                        }
                        for (int i = lastSpace; i < pt; i++)
                        {
                            inp[i] = '\0';
                        }
                        pt = lastSpace;
                        for (int i = 0; i < strlen(filteredList[0]); i++)
                        {
                            inp[pt++] = filteredList[0][i];
                            printf("%c", filteredList[0][i]);
                        }
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
                    // print filtered list
                    printf("\n");
                    for (int i = 0; i < filteredListSize; i++)
                    {
                        printf("%s\n", filteredList[i]);
                    }
                    printPrompt();
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
                        for (int i = lastSpace; i < pt; i++)
                        {
                            printf("\b \b");
                        }
                        for (int i = lastSpace; i < pt; i++)
                        {
                            inp[i] = '\0';
                        }
                        pt = lastSpace;
                        for (int i = 0; i < strlen(prefix); i++)
                        {
                            inp[pt++] = prefix[i];
                            printf("%c", prefix[i]);
                        }
                    }
                    else
                    {
                        printf("\n");
                        for (int i = 0; i < filteredListSize; i++)
                        {
                            printf("%s\n", filteredList[i]);
                        }
                    }
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
    printf("\n");
    printPrompt();
    fflush(stdout);
}

int main(void)
{
    signal(SIGCHLD, childDead);
    signal(SIGINT, dontExit);
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
            timeCorrect = 0;
            char *cmd = cmdArray[i];
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
                }
            }

            // check for output redir
            int outputIndex = -1;
            int isAppend = 0;
            char *outputRedir = strstr(cmdCopy, ">");

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

            if (strcmp(argArray[0], "exit") == 0)
            {
                exitFlag = 1;
                break;
            }
            else if (strcmp(argArray[0], "pwd") == 0)
            {
                pwd();
            }
            else if (strcmp(argArray[0], "echo") == 0)
            {
                echo(argArray, args);
            }
            else if (strcmp(argArray[0], "cd") == 0)
            {
                cd(argArray, args);
            }
            else if (strcmp(argArray[0], "ls") == 0)
            {
                ls(argArray, args);
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
            free(argArray);
            dup2(stdinCopy, STDIN_FILENO);
            dup2(stdoutCopy, STDOUT_FILENO);
        }

        free(cmdArray);

        free(in);
    }

    return 0;
}