/* 내부 명령 ln [-s] [-f] 대상 [링크이름]   (담당: 팀원 B)
 *  -s : 심볼릭 링크 (없으면 하드 링크)
 *  -f : 링크이름이 이미 있으면 지우고 만든다
 *  링크이름을 생략하거나 디렉토리이면 그 안에 대상과 같은 이름으로 만든다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include "../myshell.h"

int cmd_ln(int argc, char **argv)
{
    int c, sym = 0, force = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "sf")) != -1) {
        if (c == 's') sym = 1;
        else if (c == 'f') force = 1;
        else { fprintf(stderr, "사용법: ln [-s] [-f] 대상 [링크이름]\n"); return 2; }
    }
    int n = argc - optind;
    if (n < 1 || n > 2) { fprintf(stderr, "사용법: ln [-s] [-f] 대상 [링크이름]\n"); return 2; }

    const char *target = argv[optind];
    char *linkname;
    if (n == 1) linkname = strdup(base_name(target));
    else if (is_dir(argv[optind + 1])) linkname = join_path(argv[optind + 1], base_name(target));
    else linkname = strdup(argv[optind + 1]);

    if (force) unlink(linkname);
    int r = sym ? symlink(target, linkname) : link(target, linkname);
    if (r < 0)
        fprintf(stderr, "ln: '%s' -> '%s' 링크 생성 실패: %s\n", linkname, target, strerror(errno));
    free(linkname);
    return r < 0 ? 1 : 0;
}
