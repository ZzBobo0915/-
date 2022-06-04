#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>

int main()
{
    int shmid;

    if ((shmid = shmget((key_t)0x5005, 1024, 0666|IPC_CREAT))== -1)
    { printf("shmat(0x5005) failed\n"); return -1; }

    char* ptext = 0;
    ptext = (char*)shmat(shmid,(void*)0, 0);
    
    printf("Before write : %s\n", ptext);
    sprintf(ptext, "This program PID is : %d", getpid());
    //ptext = getpid();
    printf("After write : %s\n", ptext);

    shmdt(ptext);

    if (shmctl(shmid, IPC_RMID, 0) == -1)
    { printf("shmctl(0x5005) failed\n"); return -1; }


}
