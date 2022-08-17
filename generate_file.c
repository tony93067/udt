#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define SIZE 1000

int main()
{
    int fd, j = 0;
    fd = open("file.txt", O_RDWR|O_CREAT, S_IRWXU);
    char* data = (char*) malloc(sizeof(char)*SIZE);
    for(int i = 0; i < 500; i++)
    {
        for(int k = 0; k < 1000; k++)
        {
            memset(data, 'a' + j, SIZE);
            write(fd, data, SIZE);
            j++;
            if (j > 25)
                j = 0;
        }
    }
    close(fd);
    return 0;
}
