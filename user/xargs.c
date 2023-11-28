#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char output_buf[512];
    char **loc_argv = (char **)malloc((MAXARG + argc) * sizeof(char *));
    
    memset((void *)loc_argv, 0, (MAXARG + argc) * sizeof(char *));
    for (int i = 1; i < argc; i++)
    {
        loc_argv[i - 1] = argv[i];
    }

    if (fork() == 0)
    {
        char *p, *t;
        p = t = output_buf;

        while (read(0, p++, 1) == 1)
        {
            if (*(p - 1) == '\n')
            {
                *(p - 1) = '\0';
                int len = strlen(t);
                printf("len = %d\n", len);
                char *tem_argv = (char *)malloc(len);
                memcpy(tem_argv, t, len);

                int i;
                for (i = 1; i < argc; i++)
                {
                    loc_argv[i - 1] = argv[i];
                }
                loc_argv[i - 1] = tem_argv;

                exec(argv[1], loc_argv);
                free((void *)tem_argv);
                memset((void *)loc_argv, 0, i);

                t = p;
            }
        }
        exit(0);
    }
    
    wait((int *)0);

    exit(0);
}