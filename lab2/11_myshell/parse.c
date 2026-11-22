/* 명령줄 파싱: 따옴표, 특수 기호(| < > >> &), ~ 확장, 와일드카드(glob) 확장 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include "myshell.h"

typedef enum { T_WORD, T_PIPE, T_IN, T_OUT, T_APPEND, T_BG } TokType;

typedef struct {
    TokType type;
    char *text;
    int quoted;             /* 따옴표가 있었으면 glob 확장을 하지 않는다 */
} Token;

/* 한 줄을 토큰으로 나눈다. 반환값: 토큰 수, 오류 시 -1 */
static int tokenize(const char *s, Token *toks, int max)
{
    int n = 0;
    while (*s) {
        while (isspace((unsigned char)*s)) s++;
        if (!*s || *s == '#') break;                 /* # 이후는 주석 */
        if (n >= max) { fprintf(stderr, "myshell: 토큰이 너무 많습니다\n"); goto err; }

        Token *t = &toks[n];
        t->text = NULL;
        t->quoted = 0;
        if (*s == '|') { t->type = T_PIPE; s++; n++; continue; }
        if (*s == '<') { t->type = T_IN; s++; n++; continue; }
        if (*s == '&') { t->type = T_BG; s++; n++; continue; }
        if (*s == '>') {
            if (s[1] == '>') { t->type = T_APPEND; s += 2; }
            else { t->type = T_OUT; s++; }
            n++;
            continue;
        }

        /* 일반 단어: 따옴표 처리 */
        char buf[MAX_LINE];
        size_t len = 0;
        int had_tilde = (*s == '~');
        while (*s && !isspace((unsigned char)*s) && !strchr("|<>&", *s)) {
            if (*s == '\'' || *s == '"') {
                char q = *s++;
                t->quoted = 1;
                while (*s && *s != q) {
                    if (q == '"' && *s == '\\' && (s[1] == '"' || s[1] == '\\')) s++;
                    if (len < sizeof(buf) - 1) buf[len++] = *s;
                    s++;
                }
                if (*s != q) { fprintf(stderr, "myshell: 따옴표(%c)가 닫히지 않았습니다\n", q); goto err; }
                s++;
            } else if (*s == '\\' && s[1]) {
                s++;
                t->quoted = 1;
                if (len < sizeof(buf) - 1) buf[len++] = *s++;
            } else {
                if (len < sizeof(buf) - 1) buf[len++] = *s;
                s++;
            }
        }
        buf[len] = '\0';

        /* ~ 또는 ~/... 를 홈 디렉토리로 */
        if (had_tilde && (buf[1] == '\0' || buf[1] == '/')) {
            const char *home = getenv("HOME");
            if (home) {
                char tmp[MAX_LINE];
                snprintf(tmp, sizeof(tmp), "%s%s", home, buf + 1);
                strcpy(buf, tmp);
            }
        }
        t->type = T_WORD;
        t->text = strdup(buf);
        n++;
    }
    return n;
err:
    for (int i = 0; i < n; i++) free(toks[i].text);
    return -1;
}

static int add_arg(Command *c, const char *word)
{
    if (c->argc >= MAX_ARGS - 1) { fprintf(stderr, "myshell: 인자가 너무 많습니다\n"); return -1; }
    c->argv[c->argc++] = strdup(word);
    c->argv[c->argc] = NULL;
    return 0;
}

/* 단어에 *, ?, [ 가 있으면 glob 으로 확장한다. 일치하는 것이 없으면 그대로 둔다 (bash 와 동일) */
static int add_word(Command *c, Token *t)
{
    if (!t->quoted && strpbrk(t->text, "*?[")) {
        glob_t g;
        if (glob(t->text, 0, NULL, &g) == 0) {
            for (size_t i = 0; i < g.gl_pathc; i++)
                if (add_arg(c, g.gl_pathv[i]) < 0) { globfree(&g); return -1; }
            globfree(&g);
            return 0;
        }
    }
    return add_arg(c, t->text);
}

int parse_line(char *line, Pipeline *pl)
{
    static Token toks[MAX_ARGS * 2];
    memset(pl, 0, sizeof(*pl));
    int n = tokenize(line, toks, MAX_ARGS * 2);
    if (n < 0) goto fail_tokens;
    if (n == 0) return 1;

    pl->ncmds = 1;
    Command *cur = &pl->cmds[0];
    for (int i = 0; i < n; i++) {
        Token *t = &toks[i];
        switch (t->type) {
        case T_WORD:
            if (add_word(cur, t) < 0) goto fail;
            break;
        case T_IN: case T_OUT: case T_APPEND:
            if (i + 1 >= n || toks[i + 1].type != T_WORD) {
                fprintf(stderr, "myshell: 재지향 기호 뒤에 파일 이름이 필요합니다\n");
                goto fail;
            }
            i++;
            if (t->type == T_IN) { free(cur->infile); cur->infile = strdup(toks[i].text); }
            else {
                free(cur->outfile);
                cur->outfile = strdup(toks[i].text);
                cur->append = (t->type == T_APPEND);
            }
            break;
        case T_PIPE:
            if (cur->argc == 0 || i + 1 >= n) {
                fprintf(stderr, "myshell: '|' 근처에 문법 오류\n");
                goto fail;
            }
            if (pl->ncmds >= MAX_CMDS) { fprintf(stderr, "myshell: 파이프가 너무 많습니다\n"); goto fail; }
            cur = &pl->cmds[pl->ncmds++];
            break;
        case T_BG:
            if (i != n - 1) { fprintf(stderr, "myshell: '&' 는 명령의 마지막에만 올 수 있습니다\n"); goto fail; }
            pl->background = 1;
            break;
        }
    }
    if (cur->argc == 0) {
        if (pl->ncmds == 1 && !cur->infile && !cur->outfile) { free_pipeline(pl); goto empty; }
        fprintf(stderr, "myshell: 명령이 없습니다\n");
        goto fail;
    }
    for (int i = 0; i < n; i++) free(toks[i].text);
    return 0;

empty:
    for (int i = 0; i < n; i++) free(toks[i].text);
    return 1;
fail:
    free_pipeline(pl);
    for (int i = 0; i < n; i++) free(toks[i].text);
    return -1;
fail_tokens:
    return -1;
}

void free_pipeline(Pipeline *pl)
{
    for (int i = 0; i < pl->ncmds; i++) {
        Command *c = &pl->cmds[i];
        for (int j = 0; j < c->argc; j++) free(c->argv[j]);
        free(c->infile);
        free(c->outfile);
    }
    memset(pl, 0, sizeof(*pl));
}
