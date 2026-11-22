/* 실습 2-4: "ls -R" 과 같은 결과를 보이는 프로그램
 * 사용법: ./myls_r [-1] [디렉토리]   (생략 시 현재 디렉토리)
 * - 숨김 파일(.으로 시작)은 ls 기본 동작처럼 출력하지 않는다.
 * - 이름은 ls와 같은 순서(로케일 기준 strcoll)로 정렬한다.
 * - 심볼릭 링크 디렉토리는 따라가지 않는다(ls -R과 동일).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>
#include <sys/stat.h>

static int cmp(const void *a, const void *b)
{
    return strcoll(*(char *const *)a, *(char *const *)b);
}

static int first = 1;
static int one_per_line = 0;   /* -1 옵션: 한 줄에 하나씩 (ls -R1 과 동일) */

static void list_dir(const char *path)
{
    DIR *dp = opendir(path);
    if (!dp) {
        fprintf(stderr, "myls_r: '%s' 디렉토리를 열 수 없습니다: ", path);
        perror("");
        return;
    }

    size_t cap = 16, cnt = 0;
    char **names = malloc(cap * sizeof(char *));
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (cnt == cap) names = realloc(names, (cap *= 2) * sizeof(char *));
        names[cnt++] = strdup(de->d_name);
    }
    closedir(dp);
    qsort(names, cnt, sizeof(char *), cmp);

    if (!first) printf("\n");
    first = 0;
    printf("%s:\n", path);
    for (size_t i = 0; i < cnt; i++)
        printf("%s%s", names[i], (one_per_line || i + 1 == cnt) ? "\n" : "  ");

    /* 하위 디렉토리를 재귀적으로 방문 */
    for (size_t i = 0; i < cnt; i++) {
        char *child = malloc(strlen(path) + strlen(names[i]) + 2);
        if (path[strlen(path) - 1] == '/') sprintf(child, "%s%s", path, names[i]);
        else sprintf(child, "%s/%s", path, names[i]);
        struct stat st;
        if (lstat(child, &st) == 0 && S_ISDIR(st.st_mode))
            list_dir(child);
        free(child);
        free(names[i]);
    }
    free(names);
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    int i = 1;
    if (i < argc && strcmp(argv[i], "-1") == 0) { one_per_line = 1; i++; }
    if (i >= argc) { list_dir("."); return 0; }
    for (; i < argc; i++) list_dir(argv[i]);
    return 0;
}
