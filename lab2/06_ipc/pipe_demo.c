/* 실습 2-7 (1): 익명 파이프(pipe) 예제
 *  1) 부모 -> 자식 으로 문자열 전달
 *  2) pipe + dup2 로 "ls -1 / | wc -l" 구현
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    int fd[2];
    char buf[128];

    /* 1. 부모가 쓰고 자식이 읽는다 */
    if (pipe(fd) < 0) { perror("pipe"); return 1; }
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]);                       /* 자식: 쓰기 끝 닫기 */
        ssize_t n;
        while ((n = read(fd[0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("[자식] 파이프에서 읽음: %s", buf);
        }
        close(fd[0]);
        exit(0);
    }
    close(fd[0]);                           /* 부모: 읽기 끝 닫기 */
    const char *msgs[] = { "hello\n", "pipe IPC\n" };
    for (int i = 0; i < 2; i++) {
        write(fd[1], msgs[i], strlen(msgs[i]));
        usleep(100000);
    }
    close(fd[1]);                           /* EOF 전달 */
    waitpid(pid, NULL, 0);

    /* 2. ls -1 / | wc -l */
    printf("[부모] \"ls -1 / | wc -l\" 실행 결과: ");
    fflush(stdout);                         /* fork 전에 버퍼 비우기 */
    if (pipe(fd) < 0) { perror("pipe"); return 1; }
    if (fork() == 0) {
        dup2(fd[1], STDOUT_FILENO);         /* 표준출력 -> 파이프 */
        close(fd[0]); close(fd[1]);
        execlp("ls", "ls", "-1", "/", (char *)NULL);
        perror("exec ls"); exit(1);
    }
    if (fork() == 0) {
        dup2(fd[0], STDIN_FILENO);          /* 표준입력 <- 파이프 */
        close(fd[0]); close(fd[1]);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("exec wc"); exit(1);
    }
    close(fd[0]); close(fd[1]);
    while (wait(NULL) > 0) ;
    return 0;
}
