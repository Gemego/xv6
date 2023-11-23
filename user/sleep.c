#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        int ticks = atoi(argv[1]);
        sleep(ticks);
    }
    else
    {
        fprintf(2, "usage: sleep [num of ticks]\n");
        fprintf(2, "argc = %d\n", argc);
        exit(1);
    }
    exit(0);
}