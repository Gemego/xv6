#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char* fmtname(char *);
char* get_filename(char *);
void recur_find(char *, char *);

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        fprintf(2, "usage: find [path] [name of files]\n");
        exit(1);
    }

    recur_find(argv[1], argv[2]);
    exit(0);
}

char* fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;

    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

char* get_filename(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;

    memmove(buf, p, strlen(p));

    return buf;
}

void recur_find(char *path, char *name)
{
    char *p;
    int fd;
    struct dirent de;
    struct stat st;

    char *buf = (char *)malloc(512);

    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        exit(1);
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit(1);
    }

    switch(st.type)
    {
    case T_DEVICE:
    case T_FILE:
        if (strcmp(get_filename(path), name) == 0)
        {
            printf("%s\n", path);
        }
        break;

    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if(de.inum == 0)
                continue;
      
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            recur_find(buf, name);
            free((void *)buf);
        }
    
        break;
    }
}