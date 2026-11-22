/* 실습 2-3: 파일 및 디렉토리 관련 함수 사용 예제
 * 저수준 I/O(open/read/write/lseek/close), 파일 정보(stat), 권한(chmod),
 * 링크(link/symlink/readlink), 이름 변경(rename), 삭제(unlink),
 * 디렉토리(mkdir/opendir/readdir/rmdir/chdir/getcwd), 표준 I/O(fopen 등),
 * 장치 파일(/dev/null, /dev/urandom) 사용을 차례로 보여 준다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CHECK(expr, msg) do { if ((expr) < 0) { perror(msg); exit(EXIT_FAILURE); } } while (0)

static void print_mode(mode_t m)
{
    char s[11] = "----------";
    if (S_ISDIR(m)) s[0] = 'd';
    else if (S_ISLNK(m)) s[0] = 'l';
    else if (S_ISCHR(m)) s[0] = 'c';
    else if (S_ISBLK(m)) s[0] = 'b';
    else if (S_ISFIFO(m)) s[0] = 'p';
    else if (S_ISSOCK(m)) s[0] = 's';
    const char *rwx = "rwxrwxrwx";
    for (int i = 0; i < 9; i++)
        if (m & (1 << (8 - i))) s[i + 1] = rwx[i];
    printf("%s", s);
}

static void show_stat(const char *path)
{
    struct stat st;
    CHECK(lstat(path, &st), "lstat");
    printf("  %-22s ", path);
    print_mode(st.st_mode);
    printf("  inode=%-9lu links=%lu size=%ld\n",
           (unsigned long)st.st_ino, (unsigned long)st.st_nlink, (long)st.st_size);
}

int main(void)
{
    char buf[256];
    ssize_t n;
    int fd;

    printf("== 1. 작업 디렉토리 생성 및 이동 (mkdir, chdir, getcwd) ==\n");
    if (mkdir("work_dir", 0755) < 0 && errno != EEXIST) { perror("mkdir"); return 1; }
    CHECK(chdir("work_dir"), "chdir");
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) { perror("getcwd"); return 1; }
    printf("  현재 디렉토리: %s\n", cwd);

    printf("\n== 2. 저수준 I/O (open, write, lseek, read, close) ==\n");
    fd = open("data.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    CHECK(fd, "open");
    const char *msg = "Hello, Linux file I/O!\nSecond line.\n";
    CHECK(write(fd, msg, strlen(msg)), "write");
    off_t pos = lseek(fd, 7, SEEK_SET);          /* "Linux" 위치로 이동 */
    CHECK(pos, "lseek");
    n = read(fd, buf, 5);
    CHECK(n, "read");
    buf[n] = '\0';
    printf("  offset %ld 에서 5바이트 읽기: \"%s\"\n", (long)pos, buf);
    off_t size = lseek(fd, 0, SEEK_END);
    printf("  파일 크기(lseek SEEK_END): %ld 바이트\n", (long)size);
    close(fd);

    printf("\n== 3. 파일 정보와 권한 (stat, chmod) ==\n");
    show_stat("data.txt");
    CHECK(chmod("data.txt", 0600), "chmod");
    printf("  chmod 0600 이후:\n");
    show_stat("data.txt");

    printf("\n== 4. 링크 (link, symlink, readlink) ==\n");
    unlink("hard.txt"); unlink("soft.txt");
    CHECK(link("data.txt", "hard.txt"), "link");
    CHECK(symlink("data.txt", "soft.txt"), "symlink");
    show_stat("data.txt");
    show_stat("hard.txt");
    show_stat("soft.txt");
    n = readlink("soft.txt", buf, sizeof(buf) - 1);
    CHECK(n, "readlink");
    buf[n] = '\0';
    printf("  soft.txt -> %s\n", buf);

    printf("\n== 5. 표준 I/O (fopen, fprintf, fgets) ==\n");
    FILE *fp = fopen("note.txt", "w");
    if (!fp) { perror("fopen"); return 1; }
    for (int i = 1; i <= 3; i++) fprintf(fp, "line %d\n", i);
    fclose(fp);
    fp = fopen("note.txt", "r");
    if (!fp) { perror("fopen"); return 1; }
    while (fgets(buf, sizeof(buf), fp)) printf("  읽음: %s", buf);
    fclose(fp);

    printf("\n== 6. 이름 변경 (rename) ==\n");
    CHECK(rename("note.txt", "note_renamed.txt"), "rename");
    printf("  note.txt -> note_renamed.txt\n");

    printf("\n== 7. 디렉토리 읽기 (opendir, readdir, closedir) ==\n");
    CHECK(mkdir("sub", 0755) < 0 && errno != EEXIST ? -1 : 0, "mkdir sub");
    DIR *dp = opendir(".");
    if (!dp) { perror("opendir"); return 1; }
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        printf("  %s%s\n", de->d_name, de->d_type == DT_DIR ? "/" : "");
    }
    closedir(dp);

    printf("\n== 8. 장치 파일 (/dev/urandom, /dev/null) ==\n");
    fd = open("/dev/urandom", O_RDONLY);
    CHECK(fd, "open /dev/urandom");
    unsigned char rnd[8];
    CHECK(read(fd, rnd, sizeof(rnd)), "read urandom");
    close(fd);
    printf("  /dev/urandom 8바이트: ");
    for (int i = 0; i < 8; i++) printf("%02x", rnd[i]);
    printf("\n");
    fd = open("/dev/null", O_WRONLY);
    CHECK(fd, "open /dev/null");
    n = write(fd, "discard", 7);
    printf("  /dev/null 에 %zd 바이트 쓰기 성공 (데이터는 버려짐)\n", n);
    close(fd);
    show_stat("/dev/null");

    printf("\n== 9. 정리 (unlink, rmdir) ==\n");
    unlink("soft.txt"); unlink("hard.txt"); unlink("data.txt"); unlink("note_renamed.txt");
    CHECK(rmdir("sub"), "rmdir sub");
    CHECK(chdir(".."), "chdir ..");
    CHECK(rmdir("work_dir"), "rmdir work_dir");
    printf("  생성한 파일과 디렉토리를 모두 삭제했습니다.\n");
    return 0;
}
