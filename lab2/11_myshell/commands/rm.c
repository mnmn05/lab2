/* 내부 명령 rm [-r] [-f] 파일...   (담당: 팀원 B)
 *  -r : 디렉토리를 내용과 함께 삭제
 *  -f : 없는 파일은 무시하고 오류를 출력하지 않음
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../myshell.h"

int cmd_rm(int argc, char **argv)
{
    int c, recursive = 0, force = 0, ret = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "rRf")) != -1) {
        if (c == 'r' || c == 'R') recursive = 1;
        else if (c == 'f') force = 1;
        else { fprintf(stderr, "사용법: rm [-r] [-f] 파일...\n"); return 2; }
    }
    if (optind >= argc && !force) { fprintf(stderr, "rm: 삭제할 파일을 지정하세요\n"); return 1; }

    for (int i = optind; i < argc; i++) {
        const char *path = argv[i];
        const char *b = base_name(path);
        if (!strcmp(b, ".") || !strcmp(b, "..") || !strcmp(path, "/")) {
            fprintf(stderr, "rm: '%s' 는 삭제할 수 없습니다\n", path);
            ret = 1;
            continue;
        }
        struct stat st;
        if (lstat(path, &st) < 0) {
            if (!(force && errno == ENOENT)) {
                fprintf(stderr, "rm: '%s' 삭제 실패: %s\n", path, strerror(errno));
                ret = 1;
            }
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (!recursive) { fprintf(stderr, "rm: '%s' 는 디렉토리입니다 (-r 필요)\n", path); ret = 1; continue; }
            if (remove_tree(path) < 0) ret = 1;
        } else if (unlink(path) < 0) {
            fprintf(stderr, "rm: '%s' 삭제 실패: %s\n", path, strerror(errno));
            ret = 1;
        }
    }
    return ret;
}
