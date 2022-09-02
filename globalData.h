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
};

struct job backgroundJobs[10000];
int curbackgroundJobs;

double lastTime;