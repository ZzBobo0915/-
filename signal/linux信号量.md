# linux信号量

#### 一、信号量的概念

​    信号量（信号灯）本质上是一个计数器，用于协调多个进程（包括但不限于父子进程）对共享数据对象的读/写。它不以传送数据为目的，主要是用来保护共享资源（共享内存、消息队列、socket连接池、数据库连接池等），保证共享资源在一个时刻只有一个进程独享。

​    信号量是一个特殊的变量，只允许进程对它进行等待信号和发送信号操作。最简单的信号量是取值0和1的二元信号量，这是信号量最常见的形式。

​    通用信号量（可以取多个正整数值）和信号量集方面的知识比较复杂，应用场景也比较少。本文只介绍二元信号量。

#### 二、相关函数

​    Linux中提供了一组函数用于操作信号量，程序中需要包含以下头文件

```c++
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
```

##### 1.semget函数

​    semget函数用来获取或创建信号量，它的原型如下：

```c++
int semget(key_t key, int nsems, int semflg);
```

​    1）参数key是信号量的键值，typedef unsigned int key_t，是信号量在系统中的编号，不同信号量的编号不能相同，这一点由程序员保证。key用十六进制表示比较好。

​    2）参数nsems是创建信号量集中信号量的个数，该参数只在创建信号量集时有效，这里固定填1。

​    3）参数sem_flags是一组标志，如果希望信号量不存在时创建一个新的信号量，可以和值IPC_CREAT做按位或操作。如果没有设置IPC_CREAT标志并且信号量不存在，就会返错误（errno的值为2，No such file or directory）。

​    4）如果semget函数成功，返回信号量集的标识；失败返回-1，错误原因存于error中。

##### 2.semctl函数

​    该函数用来控制信号量（常用于设置信号量的初始值和销毁信号量），它的原型如下：

```c++
int semctl(int semid, int sem_num, int command, ...);
```

​    1）参数semid是由semget函数返回的信号量标识。

​    2）参数sem_num是信号量集数组上的下标，表示某一个信号量，填0。

​    3）参数cmd是对信号量操作的命令种类，常用的有以下两个：

​    **IPC_RMID**：销毁信号量，不需要第四个参数；

​    **SETVAL**：初始化信号量的值（信号量成功创建后，需要设置初始值），这个值由第四个参数决定。第四参数是一个自定义的共同体，如下：

```c++
// 用于信号灯操作的共同体。
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};
```

​    4）如果semctl函数调用失败返回-1；如果成功，返回值比较复杂，暂时不关心它。

​    一般有下两种例子：

```c++
//销毁信号量
semctl(semid, 0, IPC_RMID);
//初始化信号量的值为1，信号量可用
union semun sem_union;
sem_union.val = 1;
semctl(semid, 0, SETVAL, sem_union);
```

##### 3.semop函数

​    该函数有两个功能：1）等待信号量的值变为1，如果等待成功，立即把信号量的值置为0，这个过程也称之为等待锁；2）把信号量的值置为1，这个过程也称之为释放锁。

```c++
int semop(int semid, struct sembuf* sops, unsigned nsops);
```

​    1）参数semid是由semget函数返回的信号量标识。

​    2）参数nsops是操作信号量的个数，即sops结构变量的个数，设置它的为1（只对一个信号量的操作）。

​    3）参数sops是一个结构体，如下：

```c++
struct sembuf{
    short sem_num;  //信号量集的个数，单个信号量设置为0
    short sem_op;   //信号量在本次操作中需要改变的数据：-1为等待操作，1为发送操作
    short sem_flg;  //把此标志设置为SEM_UNDO，操作系统将跟踪这个信号量。
    //如果当前进程退出时没有释放信号量，操作系统将释放信号量，避免资源被死锁
}
```

​    一般有下两种例子：

```c++
//等待信号量的值变为1，如果等待成功，立即把信号量置为0
struct sembuf sem_b;
sem_b.sem_num = 0;
sem_b.sem_op = -1;
sem_b.sem_flg = SEM_UNDO;
semop(sem_id, &sem_b, 1);
//把信号量置为1
struct sembuf sem_b;
sem_b.sem_num = 0;
sem_b.sem_op = 1;
sem_b.sem_flg = SEM_UNDO;
semop(sem_id, &sem_b, 1);
```

#### 三、实例程序

​    为了便于理解，我把信号量的操作封装成CSEM类，称之为信号灯，类似互斥锁，包括初始化信号灯、等待信号灯、挂出信号灯和销毁信号灯。

##### 1.示例

```c++
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<sys/ipc.h>
#include<sys/sem.h>

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
    CSEM sem;
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
    sleep(50);  //sleep过程中，运行其他的相同程序将等待锁
    
    //挂出信号灯，释放锁
    if (sem.post() == false){
        printf("sem.post failed.\n");
        return -1;
    }
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
            if ((sem_id=segmet(key, 1, 0666|IPC_CREAT)) == -1){
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
    if (semop(semid, &sem_b, 1) == -1){
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
    if (semop(semid, &sem_b, 1) == -1){
        perror("post semop()");
        return false;
    }
    return true;
}
```

