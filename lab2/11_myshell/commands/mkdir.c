/* 내부 명령 mkdir [-p] 디렉토리...   (담당: 팀원 A) */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include "../myshell.h"

/* -p: 중간 경로까지 모두 만들고, 이미 있어도 오류가 아님 */
static int mkdir_parents(const char *path)
{
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s", path);
    for (char *p = buf + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(buf, 0755) < 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(buf, 0755) < 0 && errno != EEXIST) return -1;
    return is_dir(buf) ? 0 : (errno = ENOTDIR, -1);
}

int cmd_mkdir(int argc, char **argv)
{
    int c, parents = 0, ret = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "p")) != -1) {
        if (c == 'p') parents = 1;
        else { fprintf(stderr, "사용법: mkdir [-p] 디렉토리...\n"); return 2; }
    }
    if (optind >= argc) { fprintf(stderr, "mkdir: 디렉토리 이름이 필요합니다\n"); return 1; }
    for (int i = optind; i < argc; i++) {
        int r = parents ? mkdir_parents(argv[i]) : mkdir(argv[i], 0755);
        if (r < 0) {
            fprintf(stderr, "mkdir: '%s' 디렉토리를 만들 수 없습니다: %s\n", argv[i], strerror(errno));
            ret = 1;
        }
    }
    return ret;
}
