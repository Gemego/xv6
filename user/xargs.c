#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char output_buf[512];
    // char *loc_argv[MAXARG];

    if (fork() == 0)
    {
        char *p, *t;
        p = t = output_buf;

        while (read(0, p++, 1) == 1)
        {
            if (*(p - 1) == '\n')
            {
                *(p - 1) == '\0';
                int len = strlen(t);
                char *tem_argv = (char *)malloc(len);
                memcpy(tem_argv, t, len);

                exec(argv[1], &tem_argv);

                t = t + len + 1;
            }
        }
        exit(0);
    }
    
    wait((int *)0);

    exit(0);
}