/* 실습 2-7 (3): System V IPC 예제
 *  - 메시지 큐 (msgget / msgsnd / msgrcv / msgctl)
 *  - 공유 메모리 (shmget / shmat / shmdt / shmctl)
 *  - 세마포어 (semget / semop / semctl) 로 공유 메모리 접근 동기화
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

struct msgbuf_t { long mtype; char text[64]; };
union semun { int val; struct semid_ds *buf; unsigned short *array; };

static void sem_op(int semid, int op)
{
    struct sembuf sb = { 0, (short)op, 0 };
    if (semop(semid, &sb, 1) < 0) { perror("semop"); exit(1); }
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* ---------- 1. 메시지 큐 ---------- */
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (qid < 0) { perror("msgget"); return 1; }
    if (fork() == 0) {
        struct msgbuf_t m;
        for (int i = 0; i < 2; i++) {
            msgrcv(qid, &m, sizeof(m.text), 0, 0);
            printf("[자식] 메시지 큐 수신 (type=%ld): %s\n", m.mtype, m.text);
        }
        exit(0);
    }
    struct msgbuf_t m = { 1, "first message" };
    msgsnd(qid, &m, sizeof(m.text), 0);
    m.mtype = 2; strcpy(m.text, "second message");
    msgsnd(qid, &m, sizeof(m.text), 0);
    wait(NULL);
    msgctl(qid, IPC_RMID, NULL);

    /* ---------- 2. 공유 메모리 + 세마포어 ---------- */
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0600);
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (shmid < 0 || semid < 0) { perror("shmget/semget"); return 1; }
    union semun arg = { .val = 1 };
    semctl(semid, 0, SETVAL, arg);              /* 이진 세마포어(뮤텍스) 초기값 1 */

    int *counter = shmat(shmid, NULL, 0);
    *counter = 0;
    const int N = 4, LOOP = 10000;
    for (int p = 0; p < N; p++) {
        if (fork() == 0) {
            int *c = shmat(shmid, NULL, 0);
            for (int i = 0; i < LOOP; i++) {
                sem_op(semid, -1);              /* P */
                (*c)++;
                sem_op(semid, +1);              /* V */
            }
            shmdt(c);
            exit(0);
        }
    }
    while (wait(NULL) > 0) ;
    printf("[부모] 공유 메모리 카운터 = %d (기대값 %d)\n", *counter, N * LOOP);
    shmdt(counter);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}
