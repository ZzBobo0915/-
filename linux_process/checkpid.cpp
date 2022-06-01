#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>

int main()
{
    printf("PID = %d\n", getpid());
    sleep(30);
    
    return 0;
}

