/* 실습 2-10: POSIX 공유 메모리와 mmap 을 이용한 파일 복사
 * 사용법: ./shm_copy <원본파일> <대상파일>
 *
 *  [부모]  원본 파일을 mmap  --memcpy-->  POSIX 공유 메모리(/lab2_shm_copy)
 *                                              | sem_post (복사 완료 알림)
 *  [자식]  sem_wait  -> 공유 메모리 mmap  --memcpy-->  대상 파일 mmap  -> msync
 *
 * 공유 메모리는 청크(1MB) 단위로 사용하므로 큰 파일도 복사할 수 있다.
 * 두 개의 이름 있는 세마포어로 생산자(부모)/소비자(자식)를 동기화한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_NAME   "/lab2_shm_copy"
#define SEM_FULL   "/lab2_sem_full"
#define SEM_EMPTY  "/lab2_sem_empty"
#define CHUNK      (1024 * 1024)

struct shared {
    size_t len;                 /* 이번 청크의 길이 */
    char data[CHUNK];
};

static void die(const char *msg) { perror(msg); exit(1); }

int main(int argc, char *argv[])
{
    if (argc != 3) { fprintf(stderr, "사용법: %s <원본> <대상>\n", argv[0]); return 1; }

    /* 원본 파일 열기 및 크기 확인 */
    int src = open(argv[1], O_RDONLY);
    if (src < 0) die(argv[1]);
    struct stat st;
    if (fstat(src, &st) < 0) die("fstat");
    if (!S_ISREG(st.st_mode)) { fprintf(stderr, "%s: 일반 파일이 아닙니다\n", argv[1]); return 1; }
    size_t size = st.st_size;

    /* 대상 파일 생성: 원본과 같은 크기로 늘려 둔다 (mmap 하려면 크기가 있어야 함) */
    int dst = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (dst < 0) die(argv[2]);
    if (ftruncate(dst, size) < 0) die("ftruncate dst");
    if (size == 0) { printf("빈 파일을 복사했습니다.\n"); return 0; }

    /* POSIX 공유 메모리 생성 */
    shm_unlink(SHM_NAME);
    int shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm < 0) die("shm_open");
    if (ftruncate(shm, sizeof(struct shared)) < 0) die("ftruncate shm");

    sem_unlink(SEM_FULL); sem_unlink(SEM_EMPTY);
    sem_t *full = sem_open(SEM_FULL, O_CREAT | O_EXCL, 0600, 0);
    sem_t *empty = sem_open(SEM_EMPTY, O_CREAT | O_EXCL, 0600, 1);
    if (full == SEM_FAILED || empty == SEM_FAILED) die("sem_open");

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        /* ---------- 자식: 공유 메모리 -> 대상 파일 ---------- */
        struct shared *shp = mmap(NULL, sizeof(*shp), PROT_READ, MAP_SHARED, shm, 0);
        char *out = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dst, 0);
        if (shp == MAP_FAILED || out == MAP_FAILED) die("child mmap");
        size_t done = 0;
        while (done < size) {
            sem_wait(full);
            memcpy(out + done, shp->data, shp->len);
            done += shp->len;
            sem_post(empty);
        }
        if (msync(out, size, MS_SYNC) < 0) die("msync");
        munmap(out, size);
        munmap(shp, sizeof(*shp));
        printf("[자식 %d] 공유 메모리에서 %zu 바이트를 %s 에 기록\n", getpid(), done, argv[2]);
        exit(0);
    }

    /* ---------- 부모: 원본 파일 -> 공유 메모리 ---------- */
    char *in = mmap(NULL, size, PROT_READ, MAP_PRIVATE, src, 0);
    struct shared *shp = mmap(NULL, sizeof(*shp), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (in == MAP_FAILED || shp == MAP_FAILED) die("parent mmap");
    size_t off = 0;
    int chunks = 0;
    while (off < size) {
        sem_wait(empty);
        size_t n = size - off < CHUNK ? size - off : CHUNK;
        memcpy(shp->data, in + off, n);
        shp->len = n;
        off += n;
        chunks++;
        sem_post(full);
    }
    printf("[부모 %d] %s (%zu 바이트) 를 %d 개 청크로 공유 메모리에 전달\n", getpid(), argv[1], size, chunks);

    int status;
    waitpid(pid, &status, 0);
    munmap(in, size);
    munmap(shp, sizeof(*shp));
    close(src); close(dst); close(shm);
    shm_unlink(SHM_NAME);
    sem_close(full); sem_close(empty);
    sem_unlink(SEM_FULL); sem_unlink(SEM_EMPTY);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("복사 완료: %s -> %s\n", argv[1], argv[2]);
        return 0;
    }
    fprintf(stderr, "복사 실패\n");
    return 1;
}
