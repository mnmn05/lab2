/* 내부 명령 테이블: 이름 -> 함수 */
#include <string.h>
#include "myshell.h"

static const struct { const char *name; builtin_fn fn; } table[] = {
    { "ls", cmd_ls },       { "pwd", cmd_pwd },     { "cd", cmd_cd },
    { "mkdir", cmd_mkdir }, { "rmdir", cmd_rmdir }, { "ln", cmd_ln },
    { "cp", cmd_cp },       { "rm", cmd_rm },       { "mv", cmd_mv },
    { "cat", cmd_cat },     { "grep", cmd_grep },
};

builtin_fn find_builtin(const char *name)
{
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
        if (strcmp(table[i].name, name) == 0) return table[i].fn;
    return NULL;
}
