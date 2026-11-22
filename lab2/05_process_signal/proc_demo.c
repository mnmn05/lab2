/* 실습 2-6 (1): 프로세스 관련 함수 예제
 * getpid/getppid, fork, exec 계열, wait/waitpid, 종료 상태 확인, system
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    /* 출력이 파일/파이프로 갈 때 fork 시 버퍼가 복제되지 않도록 버퍼링 설정 */
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("[부모] PID=%d, PPID=%d\n", getpid(), getppid());

    /* 1. fork: 자식은 메모리를 복사해서 가진다 */
    int value = 100;
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        value += 1;
        printf("  [자식1] PID=%d, PPID=%d, value=%d (부모와 별도 메모리)\n", getpid(), getppid(), value);
        exit(7);                                   /* 종료 코드 7 */
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        printf("[부모] 자식1(%d) 종료, exit code=%d, 부모의 value=%d\n", pid, WEXITSTATUS(status), value);

    /* 2. fork + exec: 자식이 다른 프로그램(ls)으로 바뀐다 */
    pid = fork();
    if (pid == 0) {
        printf("  [자식2] execlp(\"ls\", \"ls\", \"-l\", \"proc_demo.c\") 실행\n");
        fflush(stdout);
        execlp("ls", "ls", "-l", "proc_demo.c", (char *)NULL);
        perror("execlp");                          /* exec 성공 시 여기에 오지 않음 */
        exit(127);
    }
    wait(&status);
    printf("[부모] 자식2 종료, exit code=%d\n", WEXITSTATUS(status));

    /* 3. 여러 자식을 만들고 모두 기다리기 */
    for (int i = 0; i < 3; i++) {
        if (fork() == 0) {
            usleep((3 - i) * 100000);              /* 늦게 만든 자식이 먼저 끝남 */
            printf("  [자식 #%d] PID=%d 작업 완료\n", i, getpid());
            exit(i);
        }
    }
    pid_t done;
    while ((done = wait(&status)) > 0)
        printf("[부모] PID %d 회수 (exit=%d)\n", done, WEXITSTATUS(status));

    /* 4. system(): 내부적으로 fork + exec("/bin/sh -c ...") + wait */
    printf("[부모] system(\"echo hello from system; exit 3\")\n");
    int r = system("echo '  hello from system'; exit 3");
    printf("[부모] system 반환 exit code=%d\n", WEXITSTATUS(r));
    return 0;
}
