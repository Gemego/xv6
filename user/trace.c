#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        uint32 mask = (uint32)atoi(argv[1]);
        trace(mask);
        
        char **tmp_argv = (char **)malloc((argc - 2) * sizeof(char *));
        for (int i = 0; i < argc - 2; i++)
        {
            tmp_argv[i] = argv[i + 2];
        }
        exec(argv[2], tmp_argv);
    }
    else
    {
        fprintf(2, "usage: trace [system calls mask] [cmd]\n");
        exit(1);
    }
    exit(0);
}