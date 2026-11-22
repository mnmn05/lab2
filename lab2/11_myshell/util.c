/* 여러 내부 명령(cp, mv, rm, ln)이 함께 쓰는 도우미 함수 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "myshell.h"

int is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

const char *base_name(const char *path)
{
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') len--;          /* 끝의 / 무시 */
    const char *p = path + len;
    while (p > path && p[-1] != '/') p--;
    static char buf[4096];
    snprintf(buf, sizeof(buf), "%.*s", (int)(path + len - p), p);
    return buf;
}

char *join_path(const char *dir, const char *name)
{
    size_t dl = strlen(dir);
    char *r = malloc(dl + strlen(name) + 2);
    if (!r) return NULL;
    if (dl > 0 && dir[dl - 1] == '/') sprintf(r, "%s%s", dir, name);
    else sprintf(r, "%s/%s", dir, name);
    return r;
}

/* 일반 파일 하나를 복사 (권한 유지) */
int copy_file(const char *src, const char *dst)
{
    struct stat ss, ds;
    if (stat(src, &ss) < 0) { fprintf(stderr, "'%s': %s\n", src, strerror(errno)); return -1; }
    if (stat(dst, &ds) == 0 && ss.st_dev == ds.st_dev && ss.st_ino == ds.st_ino) {
        fprintf(stderr, "'%s' 와 '%s' 는 같은 파일입니다\n", src, dst);
        return -1;
    }
    int in = open(src, O_RDONLY);
    if (in < 0) { fprintf(stderr, "'%s': %s\n", src, strerror(errno)); return -1; }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, ss.st_mode & 0777);
    if (out < 0) { fprintf(stderr, "'%s': %s\n", dst, strerror(errno)); close(in); return -1; }

    char buf[65536];
    ssize_t n;
    int ret = 0;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        char *p = buf;
        while (n > 0) {
            ssize_t w = write(out, p, n);
            if (w < 0) { fprintf(stderr, "'%s': %s\n", dst, strerror(errno)); ret = -1; goto done; }
            p += w; n -= w;
        }
    }
    if (n < 0) { fprintf(stderr, "'%s': %s\n", src, strerror(errno)); ret = -1; }
done:
    close(in);
    close(out);
    return ret;
}

/* 디렉토리를 통째로 복사 (cp -r, 다른 파일시스템으로의 mv) */
int copy_tree(const char *src, const char *dst)
{
    struct stat st;
    if (lstat(src, &st) < 0) { fprintf(stderr, "'%s': %s\n", src, strerror(errno)); return -1; }
    if (S_ISLNK(st.st_mode)) {
        char target[4096];
        ssize_t n = readlink(src, target, sizeof(target) - 1);
        if (n < 0) return -1;
        target[n] = '\0';
        unlink(dst);
        return symlink(target, dst);
    }
    if (!S_ISDIR(st.st_mode)) return copy_file(src, dst);

    if (mkdir(dst, st.st_mode & 0777) < 0 && errno != EEXIST) {
        fprintf(stderr, "'%s': %s\n", dst, strerror(errno));
        return -1;
    }
    /* 자기 자신의 하위로 복사하는 것 방지 (cp -r a a/b) */
    char rs[4096], rd[4096];
    if (realpath(src, rs) && realpath(dst, rd) &&
        strncmp(rd, rs, strlen(rs)) == 0 && rd[strlen(rs)] == '/') {
        fprintf(stderr, "'%s' 를 자기 자신의 하위 디렉토리 '%s' 로 복사할 수 없습니다\n", src, dst);
        rmdir(dst);
        return -1;
    }
    DIR *dp = opendir(src);
    if (!dp) { fprintf(stderr, "'%s': %s\n", src, strerror(errno)); return -1; }
    struct dirent *de;
    int ret = 0;
    while ((de = readdir(dp)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char *s = join_path(src, de->d_name), *d = join_path(dst, de->d_name);
        if (copy_tree(s, d) < 0) ret = -1;
        free(s); free(d);
    }
    closedir(dp);
    return ret;
}

/* 파일/디렉토리를 재귀적으로 삭제 (rm -r) */
int remove_tree(const char *path)
{
    struct stat st;
    if (lstat(path, &st) < 0) { fprintf(stderr, "'%s': %s\n", path, strerror(errno)); return -1; }
    if (!S_ISDIR(st.st_mode)) {
        if (unlink(path) < 0) { fprintf(stderr, "'%s': %s\n", path, strerror(errno)); return -1; }
        return 0;
    }
    DIR *dp = opendir(path);
    if (!dp) { fprintf(stderr, "'%s': %s\n", path, strerror(errno)); return -1; }
    struct dirent *de;
    int ret = 0;
    while ((de = readdir(dp)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char *child = join_path(path, de->d_name);
        if (remove_tree(child) < 0) ret = -1;
        free(child);
    }
    closedir(dp);
    if (rmdir(path) < 0) { fprintf(stderr, "'%s': %s\n", path, strerror(errno)); return -1; }
    return ret;
}
