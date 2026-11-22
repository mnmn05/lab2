/* 실습 2-8: 파이프(FIFO)를 이용한 텍스트 채팅 프로그램
 *
 * 사용법 (터미널 두 개):
 *   터미널1$ ./pipe_chat 1 철수
 *   터미널2$ ./pipe_chat 2 영희
 *
 * 이름 있는 파이프 두 개를 사용한다 (파이프는 단방향이므로).
 *   /tmp/lab2_chat_1to2 : 1번 -> 2번
 *   /tmp/lab2_chat_2to1 : 2번 -> 1번
 * 1번은 "읽기 열기 -> 쓰기 열기", 2번은 "쓰기 열기 -> 읽기 열기" 순서로 열어서
 * open() 이 서로를 기다리며 멈추는 교착 상태를 피한다.
 * poll() 로 키보드 입력과 파이프 입력을 동시에 감시한다.
 * "/quit" 입력, Ctrl-D, Ctrl-C 로 종료하며 상대방에게 종료가 알려진다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_1TO2 "/tmp/lab2_chat_1to2"
#define FIFO_2TO1 "/tmp/lab2_chat_2to1"
#define BUFSZ 1024

static volatile sig_atomic_t quit_flag = 0;
static void on_int(int sig) { (void)sig; quit_flag = 1; }

static int open_fifo(const char *path, int flags)
{
    int fd;
    while ((fd = open(path, flags)) < 0 && errno == EINTR && !quit_flag) ;
    if (fd < 0) { if (!quit_flag) perror(path); exit(1); }
    return fd;
}

/* write 가 일부만 쓰는 경우까지 처리 */
static int write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t n = write(fd, buf, len);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        buf += n; len -= n;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3 || (strcmp(argv[1], "1") && strcmp(argv[1], "2"))) {
        fprintf(stderr, "사용법: %s <1|2> <대화명>\n", argv[0]);
        return 1;
    }
    int me = atoi(argv[1]);
    const char *name = argv[2];
    setvbuf(stdout, NULL, _IONBF, 0);

    struct sigaction sa = { 0 };
    sa.sa_handler = on_int;                 /* SA_RESTART 없음: 블록된 호출이 EINTR 로 깨어남 */
    sigaction(SIGINT, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);               /* 상대가 나간 뒤 write 해도 죽지 않게 */

    if ((mkfifo(FIFO_1TO2, 0666) < 0 && errno != EEXIST) ||
        (mkfifo(FIFO_2TO1, 0666) < 0 && errno != EEXIST)) {
        perror("mkfifo");
        return 1;
    }

    printf("상대방 접속을 기다리는 중...\n");
    int rfd, wfd;
    if (me == 1) {
        rfd = open_fifo(FIFO_2TO1, O_RDONLY);
        wfd = open_fifo(FIFO_1TO2, O_WRONLY);
    } else {
        wfd = open_fifo(FIFO_2TO1, O_WRONLY);
        rfd = open_fifo(FIFO_1TO2, O_RDONLY);
    }
    printf("=== 채팅 시작 (나: %s) — 종료는 /quit ===\n", name);

    char line[BUFSZ], out[BUFSZ + 64], in[BUFSZ], kb[BUFSZ];
    size_t kblen = 0;
    struct pollfd pfd[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = rfd, .events = POLLIN },
    };
    int len = snprintf(out, sizeof(out), "*** %s 님이 입장했습니다 ***\n", name);
    write_all(wfd, out, len);

    while (!quit_flag) {
        if (poll(pfd, 2, -1) < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        /* 상대방 메시지 */
        if (pfd[1].revents & (POLLIN | POLLHUP)) {
            ssize_t n = read(rfd, in, sizeof(in) - 1);
            if (n <= 0) { printf("\n*** 상대방이 대화를 종료했습니다 ***\n"); break; }
            in[n] = '\0';
            printf("%s", in);
        }
        /* 내 키보드 입력: stdio 버퍼 대신 read() 로 직접 읽어 poll 과 어긋나지 않게 한다 */
        if (pfd[0].revents & (POLLIN | POLLHUP)) {
            ssize_t n = read(STDIN_FILENO, kb + kblen, sizeof(kb) - 1 - kblen);
            if (n <= 0) break;                                /* Ctrl-D (EOF) */
            kblen += n;
            kb[kblen] = '\0';
            int stop = 0;
            char *nl;
            while (!stop && (nl = strchr(kb, '\n')) != NULL) {
                *nl = '\0';
                snprintf(line, sizeof(line), "%s", kb);
                memmove(kb, nl + 1, kblen - (nl + 1 - kb) + 1);
                kblen -= nl + 1 - kb;
                if (strcmp(line, "/quit") == 0) { stop = 1; break; }
                if (line[0] == '\0') continue;
                len = snprintf(out, sizeof(out), "[%s] %s\n", name, line);
                if (write_all(wfd, out, len) < 0) { printf("*** 상대방 연결이 끊겼습니다 ***\n"); stop = 1; }
            }
            if (stop) break;
            if (kblen >= sizeof(kb) - 1) kblen = 0;           /* 너무 긴 줄은 버림 */
        }
    }

    len = snprintf(out, sizeof(out), "*** %s 님이 퇴장했습니다 ***\n", name);
    write_all(wfd, out, len);
    close(wfd);
    close(rfd);
    if (me == 1) { unlink(FIFO_1TO2); unlink(FIFO_2TO1); }
    printf("채팅을 종료합니다.\n");
    return 0;
}
