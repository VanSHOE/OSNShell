char *shellHome;
#ifndef MAX_BUF
#define MAX_BUF 2000
#endif
struct cmdHistory
{
    char *cmd;
    struct cmdHistory *next;
    struct cmdHistory *prev;
};

struct cmdHistory *cmdHistoryHead;

char *OLDPWD;

int curBackground;