/* 내부 명령 rmdir 디렉토리...   (담당: 팀원 B) — 비어 있는 디렉토리만 삭제 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "../myshell.h"

int cmd_rmdir(int argc, char **argv)
{
    int ret = 0;
    if (argc < 2) { fprintf(stderr, "rmdir: 디렉토리 이름이 필요합니다\n"); return 1; }
    for (int i = 1; i < argc; i++) {
        if (rmdir(argv[i]) < 0) {
            fprintf(stderr, "rmdir: '%s' 삭제 실패: %s\n", argv[i], strerror(errno));
            ret = 1;
        }
    }
    return ret;
}
