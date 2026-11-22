/* 내부 명령 grep [-i] [-v] [-n] [-c] 패턴 [파일...]   (담당: 팀원 C)
 *  패턴은 POSIX 기본 정규식(BRE)이며 regcomp/regexec 를 사용한다.
 *  -i 대소문자 무시, -v 일치하지 않는 줄, -n 줄 번호, -c 일치한 줄 수만 출력
 *  종료 코드: 일치 0, 없음 1, 오류 2 (GNU grep 과 동일)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <regex.h>
#include "../myshell.h"

static int opt_v, opt_n, opt_c;

static long grep_stream(FILE *fp, regex_t *re, const char *label)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    long lineno = 0, matches = 0;
    while ((len = getline(&line, &cap, fp)) != -1) {
        lineno++;
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        int hit = regexec(re, line, 0, NULL, 0) == 0;
        if (hit == !opt_v) {
            matches++;
            if (!opt_c) {
                if (label) printf("%s:", label);
                if (opt_n) printf("%ld:", lineno);
                printf("%s\n", line);
            }
        }
    }
    free(line);
    if (opt_c) {
        if (label) printf("%s:", label);
        printf("%ld\n", matches);
    }
    return matches;
}

int cmd_grep(int argc, char **argv)
{
    int c, flags = REG_NOSUB, err = 0;
    opt_v = opt_n = opt_c = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "ivnc")) != -1) {
        switch (c) {
        case 'i': flags |= REG_ICASE; break;
        case 'v': opt_v = 1; break;
        case 'n': opt_n = 1; break;
        case 'c': opt_c = 1; break;
        default: fprintf(stderr, "사용법: grep [-i] [-v] [-n] [-c] 패턴 [파일...]\n"); return 2;
        }
    }
    if (optind >= argc) { fprintf(stderr, "사용법: grep [-i] [-v] [-n] [-c] 패턴 [파일...]\n"); return 2; }

    regex_t re;
    int rc = regcomp(&re, argv[optind], flags);
    if (rc != 0) {
        char msg[256];
        regerror(rc, &re, msg, sizeof(msg));
        fprintf(stderr, "grep: 잘못된 패턴: %s\n", msg);
        return 2;
    }

    long total = 0;
    int nfiles = argc - optind - 1;
    if (nfiles == 0) {
        total = grep_stream(stdin, &re, NULL);
    } else {
        for (int i = optind + 1; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (!fp) { fprintf(stderr, "grep: %s: %s\n", argv[i], strerror(errno)); err = 1; continue; }
            total += grep_stream(fp, &re, nfiles > 1 ? argv[i] : NULL);
            fclose(fp);
        }
    }
    regfree(&re);
    if (err) return 2;
    return total > 0 ? 0 : 1;
}
