/* 실습 2-12 (팀과제): 간단한 쉘 — 공통 헤더 */
#ifndef MYSHELL_H
#define MYSHELL_H

#include <sys/types.h>

#define MAX_LINE  4096
#define MAX_ARGS  256
#define MAX_CMDS  32
#define MAX_JOBS  32

/* 파이프라인을 이루는 명령 하나: argv + 재지향 정보 */
typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    char *infile;           /* < file  */
    char *outfile;          /* > file 또는 >> file */
    int append;             /* >> 이면 1 */
} Command;

/* cmd1 | cmd2 | ... [&] */
typedef struct {
    Command cmds[MAX_CMDS];
    int ncmds;
    int background;         /* 마지막에 & 가 있으면 1 */
} Pipeline;

/* parse.c */
int parse_line(char *line, Pipeline *pl);   /* 0: 성공, 1: 빈 줄, -1: 문법 오류 */
void free_pipeline(Pipeline *pl);

/* builtins.c — 내부 명령 테이블 */
typedef int (*builtin_fn)(int argc, char **argv);
builtin_fn find_builtin(const char *name);

/* 각 내부 명령 (팀원별로 나누어 구현: commands/ 디렉토리) */
int cmd_ls(int argc, char **argv);
int cmd_pwd(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_rmdir(int argc, char **argv);
int cmd_ln(int argc, char **argv);
int cmd_cp(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_mv(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_grep(int argc, char **argv);

/* util.c — 명령들이 함께 쓰는 도우미 함수 */
int is_dir(const char *path);
const char *base_name(const char *path);
char *join_path(const char *dir, const char *name);  /* malloc, 호출자가 free */
int copy_file(const char *src, const char *dst);
int copy_tree(const char *src, const char *dst);
int remove_tree(const char *path);

#endif
