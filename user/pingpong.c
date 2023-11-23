#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];
    char buff[1];
    pipe(p);

    buff[0] = 'a';
    write(p[1], buff, 1);

    if (fork() == 0)
    {
        read(p[0], buff, 1);
        int id = getpid();
        printf("%d: received ping\n", id);
    }
    else if (fork() > 0)
    {
        wait((int *) 0);
        int id = getpid();
        printf("%d: received pong\n", id);
    }
    exit(0);
}