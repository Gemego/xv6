#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("%d\n", uptime());
    }
    else
    {
        fprintf(2, "usage: uptime\n");
    }
    exit(0);
}