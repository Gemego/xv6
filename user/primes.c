#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    int primes[34];

    for (int i = 2; i <= 35; i++)
    {
        primes[i - 2] = i;
    }
    
}