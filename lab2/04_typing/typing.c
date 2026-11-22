/* 실습 2-5: 타자 연습 프로그램
 * 문장을 하나씩 보여 주고 사용자가 입력한 문장과 비교하여
 *   - 잘못 타이핑한 횟수 (편집 거리: 틀린 글자 + 빠뜨린 글자 + 더 친 글자)
 *   - 평균 분당 타자수 (정확히 친 글자 수 / 걸린 분)
 * 을 측정한다.  UTF-8 한글 문장도 글자 단위로 비교한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>

#define MAXLEN 512

static const char *sentences[] = {
    "The quick brown fox jumps over the lazy dog.",
    "Linux is a free and open source operating system.",
    "A process is a program in execution.",
    "Pipes and message queues are used for IPC.",
    "리눅스 시스템 프로그래밍은 재미있다.",
};
#define NSENT (sizeof(sentences) / sizeof(sentences[0]))

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* 두 와이드 문자열의 편집 거리(Levenshtein distance) */
static int edit_distance(const wchar_t *a, const wchar_t *b)
{
    int n = wcslen(a), m = wcslen(b);
    int *prev = malloc((m + 1) * sizeof(int));
    int *cur = malloc((m + 1) * sizeof(int));
    for (int j = 0; j <= m; j++) prev[j] = j;
    for (int i = 1; i <= n; i++) {
        cur[0] = i;
        for (int j = 1; j <= m; j++) {
            int sub = prev[j - 1] + (a[i - 1] != b[j - 1]);
            int del = prev[j] + 1, ins = cur[j - 1] + 1;
            cur[j] = sub < del ? (sub < ins ? sub : ins) : (del < ins ? del : ins);
        }
        int *t = prev; prev = cur; cur = t;
    }
    int d = prev[m];
    free(prev); free(cur);
    return d;
}

/* UTF-8 문자열 -> 와이드 문자열. 변환 실패 시 바이트 단위로 복사 */
static void to_wide(const char *s, wchar_t *w, size_t wlen)
{
    if (mbstowcs(w, s, wlen) == (size_t)-1) {
        size_t i;
        for (i = 0; s[i] && i < wlen - 1; i++) w[i] = (unsigned char)s[i];
        w[i] = L'\0';
    }
    w[wlen - 1] = L'\0';
}

int main(void)
{
    setlocale(LC_ALL, "");
    if (!strstr(setlocale(LC_CTYPE, NULL), "UTF-8") && !strstr(setlocale(LC_CTYPE, NULL), "utf8"))
        setlocale(LC_CTYPE, "C.UTF-8");    /* 한글 비교를 위해 UTF-8 로케일 사용 */

    char line[MAXLEN];
    wchar_t target[MAXLEN], typed[MAXLEN];
    int total_err = 0, total_correct = 0, total_chars = 0;
    double total_time = 0;

    printf("===== 타자 연습 =====\n");
    printf("화면에 나오는 문장을 그대로 입력하고 Enter 를 누르세요.\n");
    printf("(총 %zu 문장)\n\n", NSENT);

    for (size_t i = 0; i < NSENT; i++) {
        printf("[%zu/%zu] %s\n", i + 1, NSENT, sentences[i]);
        printf("    > ");
        fflush(stdout);

        double start = now_sec();
        if (!fgets(line, sizeof(line), stdin)) { printf("\n입력이 중단되었습니다.\n"); break; }
        double elapsed = now_sec() - start;
        line[strcspn(line, "\r\n")] = '\0';

        to_wide(sentences[i], target, MAXLEN);
        to_wide(line, typed, MAXLEN);
        int len = wcslen(target);
        int err = edit_distance(target, typed);
        int correct = len - err > 0 ? len - err : 0;
        double cpm = elapsed > 0.01 ? correct / (elapsed / 60.0) : 0;

        printf("    오타 %d 회, 정확도 %.1f%%, 소요 %.2f 초, 타수 %.0f 타/분\n\n",
               err, len ? 100.0 * correct / len : 0.0, elapsed, cpm);

        total_err += err;
        total_correct += correct;
        total_chars += len;
        total_time += elapsed;
    }

    printf("===== 결과 =====\n");
    printf("총 오타 횟수     : %d 회\n", total_err);
    printf("전체 정확도      : %.1f%%\n", total_chars ? 100.0 * total_correct / total_chars : 0.0);
    printf("총 소요 시간     : %.2f 초\n", total_time);
    printf("평균 분당 타자수 : %.0f 타/분\n", total_time > 0 ? total_correct / (total_time / 60.0) : 0.0);
    return 0;
}
