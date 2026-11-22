/* 내부 명령 pwd   (담당: 팀원 A) */
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include "../myshell.h"

int cmd_pwd(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[PATH_MAX];
    if (!getcwd(buf, sizeof(buf))) { perror("pwd"); return 1; }
    printf("%s\n", buf);
    return 0;
}
