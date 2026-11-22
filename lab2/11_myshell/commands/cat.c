/* 내부 명령 cat [-n] [파일...]   (담당: 팀원 C)
 *  파일을 주지 않거나 "-" 이면 표준입력을 읽는다 (파이프/재지향과 함께 사용 가능).
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include "../myshell.h"

static int cat_stream(FILE *fp, int number, long *lineno)
{
    int ch, at_start = 1;
    while ((ch = fgetc(fp)) != EOF) {
        if (number && at_start) printf("%6ld\t", ++*lineno);
        putchar(ch);
        at_start = (ch == '\n');
    }
    return ferror(fp) ? -1 : 0;
}

int cmd_cat(int argc, char **argv)
{
    int c, number = 0, ret = 0;
    long lineno = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "n")) != -1) {
        if (c == 'n') number = 1;
        else { fprintf(stderr, "사용법: cat [-n] [파일...]\n"); return 2; }
    }
    if (optind >= argc) return cat_stream(stdin, number, &lineno) < 0;

    for (int i = optind; i < argc; i++) {
        if (strcmp(argv[i], "-") == 0) { cat_stream(stdin, number, &lineno); continue; }
        FILE *fp = fopen(argv[i], "r");
        if (!fp) { fprintf(stderr, "cat: %s: %s\n", argv[i], strerror(errno)); ret = 1; continue; }
        if (cat_stream(fp, number, &lineno) < 0) {
            fprintf(stderr, "cat: %s: %s\n", argv[i], strerror(errno));
            ret = 1;
        }
        fclose(fp);
    }
    return ret;
}
