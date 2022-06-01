#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>

int main()
{
    for (int i = 0; i < 10; ++i){
        if (fork() <= 0){
            printf("This is son process %d, PID=%d\n",i, getpid());
            sleep(1);
            break;
        }
    }

    return 0;
}
