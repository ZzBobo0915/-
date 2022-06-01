#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>

// 由父进程生成10个子进程，在子进程中显示它是第几个子进程和子进程本身的进程编号。
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
