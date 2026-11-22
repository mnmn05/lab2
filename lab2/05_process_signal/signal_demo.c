/* 실습 2-6 (2): 시그널 관련 함수 예제
 * sigaction, kill, alarm, pause, sigprocmask(블록/펜딩), SIGCHLD 처리
 * 실행 후 3초 안에 Ctrl-C 를 누르면 SIGINT 핸들러가 동작하는 것을 볼 수 있다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

static volatile sig_atomic_t got_int = 0, got_alrm = 0, got_usr1 = 0, got_chld = 0;

static void handler(int sig)
{
    /* 핸들러 안에서는 async-signal-safe 함수(write 등)만 사용 */
    char msg[64];
    int n = snprintf(msg, sizeof(msg), "    >> 시그널 %d(%s) 수신\n", sig, strsignal(sig));
    write(STDOUT_FILENO, msg, n);
    if (sig == SIGINT) got_int = 1;
    else if (sig == SIGALRM) got_alrm = 1;
    else if (sig == SIGUSR1) got_usr1 = 1;
    else if (sig == SIGCHLD) got_chld = 1;
}

static void install(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(sig, &sa, NULL) < 0) { perror("sigaction"); exit(1); }
}

int main(void)
{
    /* 출력이 파일/파이프로 갈 때 fork 시 버퍼가 복제되지 않도록 버퍼링 설정 */
    setvbuf(stdout, NULL, _IONBF, 0);
    install(SIGINT);
    install(SIGALRM);
    install(SIGUSR1);
    install(SIGCHLD);

    printf("1. alarm(1) 후 pause(): SIGALRM 을 기다림\n");
    alarm(1);
    while (!got_alrm) pause();

    printf("2. kill(getpid(), SIGUSR1): 자신에게 시그널 보내기\n");
    kill(getpid(), SIGUSR1);

    printf("3. sigprocmask 로 SIGUSR1 블록 -> 보내도 바로 전달되지 않음\n");
    sigset_t set, pend;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);
    got_usr1 = 0;
    kill(getpid(), SIGUSR1);
    sigpending(&pend);
    printf("   블록 중: SIGUSR1 pending=%d, 핸들러 실행 여부=%d\n", sigismember(&pend, SIGUSR1), got_usr1);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
    printf("   언블록 후 핸들러 실행 여부=%d\n", got_usr1);

    printf("4. 부모가 자식에게 SIGTERM 전송, 자식 종료 시 SIGCHLD 수신\n");
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGTERM, SIG_DFL);
        for (;;) pause();                   /* 시그널이 올 때까지 대기 */
    }
    usleep(200000);
    kill(pid, SIGTERM);
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status))
        printf("   자식 %d 은(는) 시그널 %d(%s) 로 종료됨\n", pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
    printf("   SIGCHLD 수신 여부=%d\n", got_chld);

    printf("5. 3초 동안 Ctrl-C(SIGINT) 를 기다립니다. (누르지 않으면 그냥 종료)\n");
    alarm(3);
    got_alrm = 0;
    while (!got_int && !got_alrm) pause();
    alarm(0);
    printf(got_int ? "   Ctrl-C 를 잡아서 정상 종료합니다.\n" : "   시간 초과, 종료합니다.\n");
    return 0;
}
