char *shellHome;
#ifndef MAX_BUF
#define MAX_BUF 2000
#endif

#define historyLen 20
char *cmdHistory[historyLen];
int curHistHead;
int curHistTail;
char *OLDPWD;

struct job
{
    int pid;
    char *name;
    char *cmd;
};

struct job backgroundJobs[10000];
int curbackgroundJobs;

double lastTime;

struct job foregroundJobs[10000];
int curforegroundJobs;

struct lsLEntry
{
    char fileType;
    char permissions[10];
    int nlink;
    char *owner;
    char *group;
    int size;
    char *time;
    char *name;
    char *path;
};
int exitFlag;
char *parsePathforHome(char *path);
char *reverseParsePath(char *path);
int callInbuilt(char *argArray[], int argc);
void stripQuotes(char *str);
