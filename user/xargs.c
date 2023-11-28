#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

void recur_exec(int argc, char *argv[], char **loc_argv, const int start, const int end);

int main(int argc, char *argv[])
{
    char output_buf[512];
    char **loc_argv = (char **)malloc(MAXARG * sizeof(char *));
    
    memset((void *)loc_argv, 0, MAXARG * sizeof(char *));
    int loc_idx = 0;

    char *p, *t;
    p = t = output_buf;

    while (read(0, p++, 1) == 1)
    {
        if (*(p - 1) == '\n')
        {
            *(p - 1) = '\0';
            int len = strlen(t);
            char *tem_argv = (char *)malloc(len);
            memcpy(tem_argv, t, len);

            loc_argv[loc_idx++] = tem_argv;

            t = p;
        }
    }
    // printf("loc_idx = %d\n", loc_idx);

    recur_exec(argc, argv, loc_argv, 0, loc_idx);
    
    wait((int *)0);

    free((void *)loc_argv);

    exit(0);
}

void recur_exec(int argc, char *argv[], char **loc_argv, const int start, const int end)
{
    if (start == end)   return;
    if (fork() > 0)
    {
        char **tmp_argv = (char **)malloc(MAXARG * sizeof(char *));
        memset((void *)tmp_argv, 0, MAXARG * sizeof(char *));
        int i;
        for (i = 1; i < argc; i++)
        {
            tmp_argv[i - 1] = argv[i];
        }
        tmp_argv[i - 1] = loc_argv[start];
        wait((int *)0);
        exec(argv[1], tmp_argv);
    }
    else
    {
        recur_exec(argc, argv, loc_argv, start + 1, end);
    }
}