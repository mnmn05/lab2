/* 내부 명령 cd [디렉토리 | - ]   (담당: 팀원 A)
 * 현재 디렉토리는 프로세스마다 따로 있으므로 cd 는 반드시 쉘(부모) 프로세스에서 실행한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include "../myshell.h"

int cmd_cd(int argc, char **argv)
{
    const char *target;
    int print = 0;
    if (argc < 2) {
        target = getenv("HOME");
        if (!target) { fprintf(stderr, "cd: HOME 이 설정되지 않았습니다\n"); return 1; }
    } else if (strcmp(argv[1], "-") == 0) {
        target = getenv("OLDPWD");
        if (!target) { fprintf(stderr, "cd: OLDPWD 가 설정되지 않았습니다\n"); return 1; }
        print = 1;
    } else {
        target = argv[1];
    }

    char old[PATH_MAX], now[PATH_MAX], tgt[PATH_MAX];
    snprintf(tgt, sizeof(tgt), "%s", target);       /* setenv 가 target 메모리를 바꿀 수 있으므로 복사 */
    if (!getcwd(old, sizeof(old))) old[0] = '\0';
    if (chdir(tgt) < 0) { fprintf(stderr, "cd: %s: %s\n", tgt, strerror(errno)); return 1; }
    if (getcwd(now, sizeof(now))) setenv("PWD", now, 1);
    if (old[0]) setenv("OLDPWD", old, 1);
    if (print) printf("%s\n", now);
    return 0;
}
