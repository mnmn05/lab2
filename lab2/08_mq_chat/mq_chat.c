/* 실습 2-9: POSIX 메시지 큐를 이용한 텍스트 채팅 프로그램
 *
 * 사용법 (터미널 두 개):
 *   터미널1$ ./mq_chat 철수 영희
 *   터미널2$ ./mq_chat 영희 철수
 *
 * 각 사용자는 자신의 수신 큐 /lab2chat_<내이름> 을 만들고,
 * 상대방의 큐 /lab2chat_<상대이름> 으로 메시지를 보낸다.
 * 리눅스에서 mqd_t 는 파일 디스크립터이므로 poll() 로 키보드와 큐를 동시에 감시한다.
 * 메시지 우선순위: 일반 대화 1, 입장/퇴장 알림 2
 * 종료: "/quit", Ctrl-D, Ctrl-C
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#define MSG_SIZE 512
#define MAX_MSG  10

static volatile sig_atomic_t quit_flag = 0;
static void on_int(int sig) { (void)sig; quit_flag = 1; }

static mqd_t peer = (mqd_t)-1;
static char my_q[128], peer_q[128];

static void send_msg(const char *text, unsigned prio, int quiet)
{
    if (peer == (mqd_t)-1) {
        peer = mq_open(peer_q, O_WRONLY | O_NONBLOCK);
        if (peer == (mqd_t)-1) { if (!quiet) printf("  (상대방이 아직 접속하지 않아 전송하지 못했습니다)\n"); return; }
    }
    if (mq_send(peer, text, strlen(text) + 1, prio) < 0) {
        if (errno == EAGAIN) printf("  (상대방 큐가 가득 찼습니다)\n");
        else { perror("mq_send"); mq_close(peer); peer = (mqd_t)-1; }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "사용법: %s <내 이름> <상대 이름>\n", argv[0]);
        return 1;
    }
    const char *me = argv[1], *other = argv[2];
    snprintf(my_q, sizeof(my_q), "/lab2chat_%s", me);
    snprintf(peer_q, sizeof(peer_q), "/lab2chat_%s", other);
    setvbuf(stdout, NULL, _IONBF, 0);

    struct sigaction sa = { 0 };
    sa.sa_handler = on_int;
    sigaction(SIGINT, &sa, NULL);

    struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = MAX_MSG, .mq_msgsize = MSG_SIZE };
    mq_unlink(my_q);                                     /* 이전 실행에서 남은 큐 제거 */
    mqd_t mine = mq_open(my_q, O_RDONLY | O_CREAT | O_NONBLOCK, 0600, &attr);
    if (mine == (mqd_t)-1) { perror("mq_open (내 큐)"); return 1; }

    printf("=== POSIX MQ 채팅 (%s -> %s) — 종료는 /quit ===\n", me, other);
    char text[MSG_SIZE];
    snprintf(text, sizeof(text), "*** %s 님이 입장했습니다 ***", me);
    send_msg(text, 2, 1);

    struct pollfd pfd[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = (int)mine, .events = POLLIN },
    };
    char line[MSG_SIZE - 64], buf[MSG_SIZE], kb[MSG_SIZE - 64];
    size_t kblen = 0;
    unsigned prio;

    while (!quit_flag) {
        if (poll(pfd, 2, -1) < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (pfd[1].revents & POLLIN) {
            ssize_t n;
            while ((n = mq_receive(mine, buf, sizeof(buf), &prio)) >= 0) {
                printf("%s\n", buf);
                if (prio == 2 && strstr(buf, "퇴장")) { mq_close(peer); peer = (mqd_t)-1; }
            }
        }
        /* 키보드 입력: read() 로 직접 읽어 여러 줄이 한 번에 와도 모두 처리 */
        if (pfd[0].revents & (POLLIN | POLLHUP)) {
            ssize_t n = read(STDIN_FILENO, kb + kblen, sizeof(kb) - 1 - kblen);
            if (n <= 0) break;                                /* Ctrl-D (EOF) */
            kblen += n;
            kb[kblen] = '\0';
            int stop = 0;
            char *nl;
            while ((nl = strchr(kb, '\n')) != NULL) {
                *nl = '\0';
                snprintf(line, sizeof(line), "%s", kb);
                memmove(kb, nl + 1, kblen - (nl + 1 - kb) + 1);
                kblen -= nl + 1 - kb;
                if (strcmp(line, "/quit") == 0) { stop = 1; break; }
                if (line[0] == '\0') continue;
                snprintf(text, sizeof(text), "[%s] %s", me, line);
                send_msg(text, 1, 0);
            }
            if (stop) break;
            if (kblen >= sizeof(kb) - 1) kblen = 0;
        }
    }

    snprintf(text, sizeof(text), "*** %s 님이 퇴장했습니다 ***", me);
    send_msg(text, 2, 1);
    if (peer != (mqd_t)-1) mq_close(peer);
    mq_close(mine);
    mq_unlink(my_q);
    printf("채팅을 종료합니다.\n");
    return 0;
}
