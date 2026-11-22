/* 내부 명령 cp [-r] 원본... 대상   (담당: 팀원 B)
 *  대상이 디렉토리이면 그 안에 같은 이름으로 복사한다.
 *  원본이 여러 개이면 대상은 반드시 디렉토리여야 한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "../myshell.h"

int cmd_cp(int argc, char **argv)
{
    int c, recursive = 0, ret = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "rR")) != -1) {
        if (c == 'r' || c == 'R') recursive = 1;
        else { fprintf(stderr, "사용법: cp [-r] 원본... 대상\n"); return 2; }
    }
    int n = argc - optind;
    if (n < 2) { fprintf(stderr, "사용법: cp [-r] 원본... 대상\n"); return 2; }

    const char *dst = argv[argc - 1];
    int dst_is_dir = is_dir(dst);
    if (n > 2 && !dst_is_dir) { fprintf(stderr, "cp: 대상 '%s' 는 디렉토리가 아닙니다\n", dst); return 1; }

    for (int i = optind; i < argc - 1; i++) {
        const char *src = argv[i];
        if (is_dir(src) && !recursive) {
            fprintf(stderr, "cp: -r 옵션이 없어 디렉토리 '%s' 를 건너뜁니다\n", src);
            ret = 1;
            continue;
        }
        char *target = dst_is_dir ? join_path(dst, base_name(src)) : strdup(dst);
        int r = is_dir(src) ? copy_tree(src, target) : copy_file(src, target);
        if (r < 0) ret = 1;
        free(target);
    }
    return ret;
}
