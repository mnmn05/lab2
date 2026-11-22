/* 내부 명령 mv 원본... 대상   (담당: 팀원 C)
 *  같은 파일시스템이면 rename() 한 번으로 끝난다.
 *  다른 파일시스템이면(EXDEV, 예: WSL 의 /mnt/c) 복사 후 원본을 삭제한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../myshell.h"

static int move_one(const char *src, const char *dst)
{
    if (rename(src, dst) == 0) return 0;
    if (errno != EXDEV) {
        fprintf(stderr, "mv: '%s' -> '%s' 이동 실패: %s\n", src, dst, strerror(errno));
        return -1;
    }
    if (copy_tree(src, dst) < 0) return -1;      /* 파일이면 copy_tree 가 copy_file 을 호출 */
    return remove_tree(src);
}

int cmd_mv(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "사용법: mv 원본... 대상\n"); return 2; }
    const char *dst = argv[argc - 1];
    int dst_is_dir = is_dir(dst);
    if (argc > 3 && !dst_is_dir) { fprintf(stderr, "mv: 대상 '%s' 는 디렉토리가 아닙니다\n", dst); return 1; }

    int ret = 0;
    for (int i = 1; i < argc - 1; i++) {
        char *target = dst_is_dir ? join_path(dst, base_name(argv[i])) : strdup(dst);
        if (move_one(argv[i], target) < 0) ret = 1;
        free(target);
    }
    return ret;
}
