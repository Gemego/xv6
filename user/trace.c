#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        uint32 mask = (uint32)atoi(argv[1]);
        trace(mask);
    }
    else
    {
        fprintf(2, "usage: trace [system calls mask] [cmd]\n");
        exit(1);
    }
    exit(0);
}