#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void eliminate(int);

int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    for (int i = 2; i <= 35; i++)
    {
        if (write(p[1], (char *)&i, 4) != 4)
            exit(1);
    }

    close(p[1]);
    eliminate(p[0]);

    exit(0);
}

void eliminate(int fd)
{
    int p1[2];
    pipe(p1);

    int seive_num;
    int this_prim;
    int count;
    if (read(fd, (char *)&seive_num, 4) == 4)
    {
        printf("prime %d\n", seive_num);
        this_prim = seive_num;
    }
    while (read(fd, (char *)&seive_num, 4) == 4)
    {
        if (seive_num % this_prim != 0)
            write(p1[1], (char *)&seive_num, 4);
        count++;
    }
    close(p1[1]);
    close(fd);

    if (count > 0)
    {
        eliminate(p1[0]);
    }   
}