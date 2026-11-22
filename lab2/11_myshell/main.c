/* 실습 2-12 (팀과제): 간단한 쉘 myshell
 *
 * 지원 기능
 *  1. exit [n]                : 쉘 종료 (Ctrl-D 도 종료)
 *  2. cmd &                   : 백그라운드 실행, 끝나면 "[n] Done" 출력, jobs 로 확인
 *  3. Ctrl-C(SIGINT), Ctrl-\(SIGQUIT) : 실행 중인 전경 명령만 종료, 쉘은 계속 동작
 *     Ctrl-Z(SIGTSTP)         : 전경 명령 일시정지 -> fg 로 재개
 *  4. cmd > file, cmd >> file, cmd < file, cmd1 | cmd2 | ...
 *  5. 내부 명령: ls pwd cd mkdir rmdir ln cp rm mv cat grep (commands 디렉토리)
 *     그 밖의 명령은 execvp 로 외부 프로그램을 실행한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <stdio_ext.h>
#include <sys/wait.h>
#include "myshell.h"

/* ---------------- 작업(job) 관리 ---------------- */
typedef enum { JOB_FREE = 0, JOB_RUNNING, JOB_STOPPED } JobState;
typedef struct {
    JobState state;
    pid_t pgid;              /* 작업의 프로세스 그룹 */
    pid_t pids[MAX_CMDS];
    int npids;               /* 아직 끝나지 않은 프로세스 수 */
    int total;
    char cmdline[256];
} Job;

static Job jobs[MAX_JOBS + 1];          /* 1번부터 사용 */
static int last_status = 0;
static int interactive = 0;
static pid_t shell_pgid;

static int add_job(pid_t pgid, pid_t *pids, int n, JobState st, const char *cmdline)
{
    for (int i = 1; i <= MAX_JOBS; i++) {
        if (jobs[i].state == JOB_FREE) {
            jobs[i].state = st;
            jobs[i].pgid = pgid;
            memcpy(jobs[i].pids, pids, n * sizeof(pid_t));
            jobs[i].npids = jobs[i].total = n;
            snprintf(jobs[i].cmdline, sizeof(jobs[i].cmdline), "%s", cmdline);
            return i;
        }
    }
    fprintf(stderr, "myshell: 작업 테이블이 가득 찼습니다\n");
    return -1;
}

/* 종료된 pid 를 작업 테이블에서 지운다 */
static void job_pid_done(pid_t pid, int *job_finished)
{
    *job_finished = 0;
    for (int i = 1; i <= MAX_JOBS; i++) {
        if (jobs[i].state == JOB_FREE) continue;
        for (int k = 0; k < jobs[i].total; k++) {
            if (jobs[i].pids[k] == pid) {
                jobs[i].pids[k] = -1;
                if (--jobs[i].npids == 0) *job_finished = i;
                return;
            }
        }
    }
}

/* 프롬프트를 출력하기 전에 끝난 백그라운드 작업을 회수 (좀비 방지) */
static void reap_background(void)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int fin;
        job_pid_done(pid, &fin);
        if (fin) {
            printf("[%d]+  Done                    %s\n", fin, jobs[fin].cmdline);
            jobs[fin].state = JOB_FREE;
        }
    }
}

/* 전경 작업이 끝나거나 멈출 때까지 기다린다 */
static void wait_foreground(pid_t pgid, pid_t *pids, int n, const char *cmdline)
{
    /* 터미널의 전경 프로세스 그룹을 작업에게 넘겨준다 → Ctrl-C/Ctrl-\/Ctrl-Z 가 작업에게만 전달됨 */
    if (interactive) tcsetpgrp(STDIN_FILENO, pgid);

    int stopped = 0, status = 0;
    pid_t remain[MAX_CMDS];
    int nremain = 0;

    for (int i = 0; i < n; i++) {
        pid_t r;
        do {
            r = waitpid(pids[i], &status, WUNTRACED);
        } while (r < 0 && errno == EINTR);
        if (r < 0) continue;

        if (WIFSTOPPED(status)) {
            stopped = 1;
            remain[nremain++] = pids[i];
        } else if (i == n - 1) {
            /* 파이프라인의 종료 상태는 마지막 명령의 상태 */
            if (WIFEXITED(status)) last_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status);
                if (WTERMSIG(status) == SIGQUIT) printf("Quit\n");
                else if (WTERMSIG(status) == SIGINT) printf("\n");
            }
        }
    }
    /* 터미널을 쉘에게 되돌린다 */
    if (interactive) tcsetpgrp(STDIN_FILENO, shell_pgid);
    if (stopped) {
        int id = add_job(pgid, remain, nremain, JOB_STOPPED, cmdline);
        if (id > 0) printf("\n[%d]+  Stopped                 %s\n", id, cmdline);
        last_status = 128 + SIGTSTP;
    }
}

/* ---------------- 쉘 자체의 시그널 처리 ---------------- */
static void sigint_handler(int sig)
{
    (void)sig;
    /* 프롬프트에서 Ctrl-C: 입력 줄을 버리고 새 프롬프트 (main 루프에서 처리) */
    write(STDOUT_FILENO, "\n", 1);
}

static void setup_shell_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                        /* SA_RESTART 없음 → fgets 가 EINTR 로 깨어남 */
    sigaction(SIGINT, &sa, NULL);
    signal(SIGQUIT, SIG_IGN);               /* 쉘은 Ctrl-\, Ctrl-Z 로 죽거나 멈추지 않는다 */
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

/* 자식 프로세스에서: 시그널을 기본 동작으로 되돌린다.
 * 단, 작업 제어가 없는(비대화형) 모드의 백그라운드 작업은 Ctrl-C 등을 무시하게 한다. */
static void setup_child_signals(int background)
{
    if (background && !interactive) {
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
    } else {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
    }
    signal(SIGTTOU, SIG_DFL);
}

/* ---------------- 쉘 내부에서만 실행하는 명령 (부모 프로세스 상태를 바꿈) ---------------- */
static int builtin_exit(int argc, char **argv)
{
    int code = argc > 1 ? atoi(argv[1]) : last_status;
    for (int i = 1; i <= MAX_JOBS; i++)
        if (jobs[i].state == JOB_STOPPED)          /* 멈춘 작업은 정리하고 나간다 */
            for (int k = 0; k < jobs[i].total; k++)
                if (jobs[i].pids[k] > 0) { kill(jobs[i].pids[k], SIGTERM); kill(jobs[i].pids[k], SIGCONT); }
    if (interactive) printf("exit\n");
    exit(code);
}

static int builtin_jobs(int argc, char **argv)
{
    (void)argc; (void)argv;
    for (int i = 1; i <= MAX_JOBS; i++)
        if (jobs[i].state != JOB_FREE)
            printf("[%d]  %-22s %s\n", i, jobs[i].state == JOB_RUNNING ? "Running" : "Stopped",
                   jobs[i].cmdline);
    return 0;
}

static int builtin_fg(int argc, char **argv)
{
    int id = 0;
    if (argc > 1) id = atoi(argv[1][0] == '%' ? argv[1] + 1 : argv[1]);
    else for (int i = MAX_JOBS; i >= 1; i--) if (jobs[i].state != JOB_FREE) { id = i; break; }
    if (id < 1 || id > MAX_JOBS || jobs[id].state == JOB_FREE) {
        fprintf(stderr, "fg: 해당 작업이 없습니다\n");
        return 1;
    }
    Job j = jobs[id];
    jobs[id].state = JOB_FREE;
    printf("%s\n", j.cmdline);
    pid_t pids[MAX_CMDS];
    int n = 0;
    for (int k = 0; k < j.total; k++)
        if (j.pids[k] > 0) pids[n++] = j.pids[k];
    if (interactive) tcsetpgrp(STDIN_FILENO, j.pgid);
    kill(-j.pgid, SIGCONT);                         /* 그룹 전체를 재개 */
    wait_foreground(j.pgid, pids, n, j.cmdline);
    return last_status;
}

static void builtin_help(void)
{
    printf("myshell 내부 명령\n"
           "  ls [-a] [-l] [경로...]     pwd                  cd [디렉토리|-]\n"
           "  mkdir [-p] 디렉토리...     rmdir 디렉토리...\n"
           "  ln [-s] [-f] 대상 [링크]   cp [-r] 원본... 대상  rm [-r] [-f] 파일...\n"
           "  mv 원본... 대상            cat [-n] [파일...]    grep [-i] [-v] [-n] [-c] 패턴 [파일...]\n"
           "  jobs   fg [%%n]   help   exit [n]\n"
           "기능: cmd &, cmd > f, cmd >> f, cmd < f, cmd1 | cmd2, Ctrl-C / Ctrl-\\ / Ctrl-Z\n");
}

/* 부모 프로세스에서 직접 실행해야 하는 명령인가? (파이프 없는 단일 명령일 때만) */
static int run_shell_builtin(Command *c)
{
    const char *name = c->argv[0];
    if (strcmp(name, "exit") == 0) return builtin_exit(c->argc, c->argv);
    if (strcmp(name, "cd") == 0) { last_status = cmd_cd(c->argc, c->argv); return 1; }
    if (strcmp(name, "jobs") == 0) { last_status = builtin_jobs(c->argc, c->argv); return 1; }
    if (strcmp(name, "fg") == 0) { builtin_fg(c->argc, c->argv); return 1; }
    if (strcmp(name, "help") == 0) { builtin_help(); last_status = 0; return 1; }
    return 0;
}

/* ---------------- 명령 실행 ---------------- */
static int redirect(Command *c)
{
    if (c->infile) {
        int fd = open(c->infile, O_RDONLY);
        if (fd < 0) { fprintf(stderr, "myshell: %s: %s\n", c->infile, strerror(errno)); return -1; }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (c->outfile) {
        int flags = O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC);
        int fd = open(c->outfile, flags, 0644);
        if (fd < 0) { fprintf(stderr, "myshell: %s: %s\n", c->outfile, strerror(errno)); return -1; }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    return 0;
}

static void run_pipeline(Pipeline *pl, const char *cmdline)
{
    if (pl->ncmds == 1 && !pl->background && run_shell_builtin(&pl->cmds[0]))
        return;

    pid_t pids[MAX_CMDS];
    pid_t pgid = 0;
    int prev_read = -1;
    fflush(stdout);

    for (int i = 0; i < pl->ncmds; i++) {
        Command *c = &pl->cmds[i];
        int fd[2] = { -1, -1 };
        if (i < pl->ncmds - 1 && pipe(fd) < 0) { perror("pipe"); break; }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); break; }

        if (pid == 0) {
            /* ----- 자식 ----- */
            /* 작업마다 별도 프로세스 그룹 (첫 명령의 pid 가 그룹 id) */
            setpgid(0, pgid);
            if (interactive && !pl->background) tcsetpgrp(STDIN_FILENO, getpgrp());
            setup_child_signals(pl->background);

            if (prev_read != -1) { dup2(prev_read, STDIN_FILENO); close(prev_read); }
            if (fd[1] != -1) { dup2(fd[1], STDOUT_FILENO); close(fd[0]); close(fd[1]); }
            if (pl->background && i == 0 && !c->infile) {
                int null = open("/dev/null", O_RDONLY);   /* 백그라운드는 키보드를 읽지 않음 */
                if (null >= 0) { dup2(null, STDIN_FILENO); close(null); }
            }
            if (redirect(c) < 0) _exit(1);
            /* 부모의 fgets 가 미리 읽어 둔 stdin 버퍼를 비운다.
             * (비우지 않으면 내부 명령 cat 등이 재지향된 파일 대신 쉘 입력을 읽게 된다) */
            __fpurge(stdin);

            builtin_fn fn = find_builtin(c->argv[0]);
            if (fn) {
                int r = fn(c->argc, c->argv);
                fflush(stdout);
                _exit(r);
            }
            execvp(c->argv[0], c->argv);
            fprintf(stderr, "myshell: %s: %s\n", c->argv[0],
                    errno == ENOENT ? "명령을 찾을 수 없습니다" : strerror(errno));
            _exit(127);
        }

        /* ----- 부모 ----- */
        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);                     /* 경쟁 상태를 막기 위해 부모도 설정 */
        pids[i] = pid;
        if (prev_read != -1) close(prev_read);
        if (fd[1] != -1) close(fd[1]);
        prev_read = fd[0];
    }
    if (prev_read != -1) close(prev_read);

    if (pl->background) {
        int id = add_job(pgid, pids, pl->ncmds, JOB_RUNNING, cmdline);
        if (id > 0) printf("[%d] %d\n", id, pids[pl->ncmds - 1]);
        last_status = 0;
    } else {
        wait_foreground(pgid, pids, pl->ncmds, cmdline);
    }
}

int main(void)
{
    char line[MAX_LINE];
    interactive = isatty(STDIN_FILENO);
    setup_shell_signals();
    if (interactive) {
        setpgid(0, 0);                          /* 쉘 자신의 프로세스 그룹 */
        shell_pgid = getpgrp();
        tcsetpgrp(STDIN_FILENO, shell_pgid);    /* 터미널의 전경 그룹으로 */
    }
    setenv("SHELL", "myshell", 1);

    for (;;) {
        reap_background();
        if (interactive) {
            char cwd[PATH_MAX];
            if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
            const char *home = getenv("HOME");
            size_t hl = home ? strlen(home) : 0;
            if (home && hl > 1 && strncmp(cwd, home, hl) == 0 && (cwd[hl] == '/' || cwd[hl] == '\0'))
                printf("myshell:~%s$ ", cwd + hl);
            else
                printf("myshell:%s$ ", cwd);
            fflush(stdout);
        }

        errno = 0;
        if (!fgets(line, sizeof(line), stdin)) {
            if (errno == EINTR) { clearerr(stdin); continue; }   /* Ctrl-C at prompt */
            if (interactive) printf("exit\n");
            break;                                                /* Ctrl-D (EOF) */
        }
        line[strcspn(line, "\n")] = '\0';

        char cmdline[256];
        size_t cl = strlen(line) < sizeof(cmdline) - 1 ? strlen(line) : sizeof(cmdline) - 1;
        memcpy(cmdline, line, cl);           /* 작업 목록에 보여 줄 명령 문자열 */
        cmdline[cl] = '\0';
        Pipeline pl;
        int r = parse_line(line, &pl);
        if (r == 1) continue;
        if (r < 0) { last_status = 2; continue; }
        run_pipeline(&pl, cmdline);
        free_pipeline(&pl);
    }
    return last_status;
}
