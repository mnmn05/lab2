/* 실습 2-7 (2): 이름 있는 파이프(FIFO) 예제
 * 사용법: ./fifo_demo          (한 프로그램 안에서 fork 로 송수신 시연)
 *         ./fifo_demo write    (다른 터미널에서 ./fifo_demo read 와 함께 사용)
 *         ./fifo_demo read
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/lab2_fifo"

static void writer(void)
{
    int fd = open(FIFO_PATH, O_WRONLY);     /* 읽는 쪽이 열 때까지 블록 */
    if (fd < 0) { perror("open"); exit(1); }
    for (int i = 1; i <= 3; i++) {
        char msg[64];
        int n = snprintf(msg, sizeof(msg), "FIFO message %d\n", i);
        write(fd, msg, n);
        printf("[writer] 보냄: %s", msg);
        usleep(100000);
    }
    close(fd);
}

static void reader(void)
{
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }
    char buf[128];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("[reader] 받음: %s", buf);
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }

    if (argc > 1 && strcmp(argv[1], "write") == 0) { writer(); return 0; }
    if (argc > 1 && strcmp(argv[1], "read") == 0) { reader(); unlink(FIFO_PATH); return 0; }

    pid_t pid = fork();
    if (pid == 0) { reader(); exit(0); }
    writer();
    waitpid(pid, NULL, 0);
    unlink(FIFO_PATH);
    return 0;
}
