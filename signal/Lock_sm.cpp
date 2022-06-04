#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>

class CSEM{
private:
    //用于引号灯操作的共同体
    union semun{
        int val;
        struct semid_ds* buf;
    	unsigned short* array;
    };  
  
    int sem_id;  //信号灯描述
public:
    bool init(key_t key);  //如果信号灯已存在，获取信号灯，如果不存在创建并初始化信号灯
    bool wait();  //等待信号灯挂出
    bool post();  //挂出信号灯
    bool destroy();  //销毁信号灯
};

int main(int argc, char* argv[]){
    int shmid;
    CSEM sem;

    if ((shmid = shmget((key_t)0x5000, 1024, 0666|IPC_CREAT))== -1)
    { printf("shmat(0x5000) failed\n"); return -1; }

    char* ptext = 0;
    ptext = (char*)shmat(shmid,(void*)0, 0);

    //初始化信号灯
    if (sem.init(0x5000) == false){
        printf("sem.init failed.\n");
        return -1;
    }
    printf("sem.init ok!\n");
    
    //等待信号灯挂出，等待成功后，将持有锁
    if (sem.wait() == false){
        printf("sem.wait failed.\n");
        return -1;
    }
    printf("sem.wait ok!\n");
    
    printf("Before write : %s\n", ptext);
    strcpy(ptext, argv[1]);
    sleep(10);  //sleep过程中，运行其他的相同程序将等待锁
    printf("After write : %s\n", ptext);

    if (shmctl(shmid, IPC_RMID, 0) == -1)
    { printf("shmctl(0x5005) failed\n"); return -1; }

    //挂出信号灯，释放锁
    if (sem.post() == false){
        printf("sem.post failed.\n");
        return -1;
    }
    shmdt(ptext);
    printf("sem.post ok!\n");
    
    
    //销毁信号灯
    if (sem.destroy() == false){
        printf("sem.destroy failed.\n");
        return -1;
    }
    printf("sem.destroy ok!\n");
    
    }

bool CSEM::init(key_t key){
    //获取信号灯
    if ((sem_id = semget(key, 1, 0640)) == -1){
        //如果信号灯不存在，创建它
        if (errno == 2){
            if ((sem_id=semget(key, 1, 0666|IPC_CREAT)) == -1){
                perror("init 1 semget()");
                return false;
              }
            //信号灯创建成功后，还需要把它初始化成可用的状态
            union semun sem_union;
            sem_union.val = 1;
            if (semctl(sem_id, 0, SETVAL, sem_union) < 0){
                perror("init semctl()"); 
                return false;
            }
        }else{
            perror("intt 2 semget()"); 
            return false;
        }
    }
    return true;
}

bool CSEM::destroy(){
    if (semctl(sem_id, 0, IPC_RMID) == -1){
        perror("destory semctl()");
        return false;
    }
    return true;
}

bool CSEM::wait(){
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1){
        perror("wait semop()");
        return false;
    }
    return true;
}

bool CSEM::post(){
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1){
        perror("post semop()");
        return false;
    }
    return true;
}
