#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char buf[512];
    if (fork() == 0)
    {
        // read()
    }
    
    wait((int *)0);

    exit(0);
}