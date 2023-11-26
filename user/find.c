#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        fprintf(2, "usage: find [name of files]\n");
        exit(1);
    }

    char *path = argv[1];
    char *name = argv[2];

    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "ls: cannot open %s\n", path);
        exit(1);
    }

    
}