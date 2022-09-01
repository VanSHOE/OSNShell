char *shellHome;
#ifndef MAX_BUF
#define MAX_BUF 2000
#endif

#define historyLen 20
char *cmdHistory[historyLen];
int curHistHead;
char *OLDPWD;

int curBackground;