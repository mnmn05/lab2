/* 내부 명령 ls [-a] [-l] [경로...]   (담당: 팀원 A) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../myshell.h"

static int opt_all, opt_long;

static void mode_str(mode_t m, char *s)
{
    s[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISCHR(m) ? 'c' : S_ISBLK(m) ? 'b' :
           S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
    const char *rwx = "rwxrwxrwx";
    for (int i = 0; i < 9; i++) s[i + 1] = (m & (1 << (8 - i))) ? rwx[i] : '-';
    s[10] = '\0';
}

/* path: stat 할 실제 경로, name: 화면에 보일 이름 */
static void print_entry(const char *path, const char *name)
{
    if (!opt_long) { printf("%s\n", name); return; }
    struct stat st;
    if (lstat(path, &st) < 0) { fprintf(stderr, "ls: '%s': %s\n", path, strerror(errno)); return; }
    char mode[11], tbuf[32];
    mode_str(st.st_mode, mode);
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    strftime(tbuf, sizeof(tbuf), "%b %e %H:%M", localtime(&st.st_mtime));
    printf("%s %2lu %-8s %-8s %8ld %s %s", mode, (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "?", gr ? gr->gr_name : "?", (long)st.st_size, tbuf, name);
    if (S_ISLNK(st.st_mode)) {
        char target[4096];
        ssize_t n = readlink(path, target, sizeof(target) - 1);
        if (n >= 0) { target[n] = '\0'; printf(" -> %s", target); }
    }
    printf("\n");
}

static int cmp(const void *a, const void *b) { return strcmp(*(char **)a, *(char **)b); }

static int list_dir(const char *dir)
{
    DIR *dp = opendir(dir);
    if (!dp) { fprintf(stderr, "ls: '%s': %s\n", dir, strerror(errno)); return 1; }
    char **names = NULL;
    size_t n = 0, cap = 0;
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (!opt_all && de->d_name[0] == '.') continue;
        if (n == cap) names = realloc(names, (cap = cap ? cap * 2 : 32) * sizeof(char *));
        names[n++] = strdup(de->d_name);
    }
    closedir(dp);
    qsort(names, n, sizeof(char *), cmp);
    for (size_t i = 0; i < n; i++) {
        char *p = join_path(dir, names[i]);
        print_entry(p, names[i]);
        free(p);
        free(names[i]);
    }
    free(names);
    return 0;
}

int cmd_ls(int argc, char **argv)
{
    int c, ret = 0;
    opt_all = opt_long = 0;
    optind = 1;
    while ((c = getopt(argc, argv, "al")) != -1) {
        if (c == 'a') opt_all = 1;
        else if (c == 'l') opt_long = 1;
        else { fprintf(stderr, "사용법: ls [-a] [-l] [경로...]\n"); return 2; }
    }
    if (optind >= argc) return list_dir(".");

    int nargs = argc - optind;
    for (int i = optind; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) < 0) {
            fprintf(stderr, "ls: '%s' 에 접근할 수 없습니다: %s\n", argv[i], strerror(errno));
            ret = 2;
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (nargs > 1) printf("%s:\n", argv[i]);
            ret |= list_dir(argv[i]);
            if (nargs > 1 && i < argc - 1) printf("\n");
        } else {
            print_entry(argv[i], argv[i]);
        }
    }
    return ret;
}
