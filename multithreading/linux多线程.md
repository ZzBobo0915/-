# linux多线程

#### 一、线程的概念

​    和多进程相比，多线程是一种比较节省资源的多任务操作方式。启动一个新的进程必须分配给它独立的地址空间，每个进程都有自己的堆栈段和数据段，系统开销比较高，进行数据的传递只能通过进行间通信的方式进行。在同一个进程中，可以运行多个线程，运行于同一个进程中的多个线程，它们彼此之间使用相同的地址空间，共享全局变量和对象，启动一个线程所消耗的资源比启动一个进程所消耗的资源要少。

#### 二、线程的使用

##### 1.创建线程

​    在Linux下，采用pthread_create函数来创建一个新的线程，函数声明：

```c++
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,void * (*start_routine) (void *), void *arg);
```

​    参数thread为为指向线程[标识符](http://baike.baidu.com/view/390932.htm)的地址。

​    参数attr用于设置线程属性，一般为空，表示使用默认属性。

​    参数start_routine是线程运行函数的地址，填函数名就可以了。

​    参数arg是线程运行函数的参数。新创建的线程从start_routine函数的地址开始运行，该函数只有一个无类型指针参数arg。若要想向start_routine传递多个参数，可以将多个参数放在一个结构体中，然后把结构体的地址作为arg参数传入，**但是要非常慎重，程序员一般不会这么做。**

​    在编译时注意加上-lpthread参数，以调用静态链接库。因为pthread并非Linux系统的默认库。

##### 2.线程的中止

​    如果进程中的任一线程调用了exit，则整个进程会终止，所以，在线程的start_routine函数中，不能采用exit。

​    线程的终止有三种方式：

​    1）线程的start_routine函数代码结束，自然消亡。

​    2）线程的start_routine函数调用pthread_exit结束。pthread_exit函数的声明如下：

```c++
void pthread_exit(void *retval);
```

​    参数retval填空，即0。

​    3）被主进程或其它线程中止。

##### 3.多线程的socket服务端

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
class CTcpServer
{
public:
  int m_listenfd;   // 服务端用于监听的socket
  int m_clientfd;   // 客户端连上来的socket
 
  CTcpServer();
 
  bool InitServer(int port);  // 初始化服务端
 
  bool Accept();  // 等待客户端的连接
 
  // 向对端发送报文
  int  Send(const void *buf,const int buflen);
  // 接收对端的报文
  int  Recv(void *buf,const int buflen);
 
  // void CloseClient();    // 关闭客户端的socket，多线程服务端不需要这个函数。
  // void CloseListen();    // 关闭用于监听的socket，多线程服务端不需要这个函数。
 
 ~CTcpServer();
};
 
CTcpServer TcpServer;
 
// SIGINT和SIGTERM的处理函数
void EXIT(int sig)
{
  printf("程序退出，信号值=%d\n",sig);
 
  close(TcpServer.m_listenfd);  // 手动关闭m_listenfd，释放资源
 
  exit(0);
}
 
// 与客户端通信线程的主函数
void *pth_main(void *arg);
 
int main()
{
  // 忽略全部的信号
  for (int ii=0;ii<50;ii++) signal(ii,SIG_IGN);
 
  // 设置SIGINT和SIGTERM的处理函数
  signal(SIGINT,EXIT); signal(SIGTERM,EXIT);
 
  if (TcpServer.InitServer(5051)==false)
  { printf("服务端初始化失败，程序退出。\n"); return -1; }
 
  while (1)
  {
    if (TcpServer.Accept() == false) continue;
 
    pthread_t pthid;   // 创建一线程，与新连接上来的客户端通信
    if (pthread_create(&pthid,NULL,pth_main,(void*)((long)TcpServer.m_clientfd))!=0)
    { printf("创建线程失败，程序退出。n"); return -1; }
 
    printf("与客户端通信的线程已创建。\n");
  }
}
 
CTcpServer::CTcpServer()
{
  // 构造函数初始化socket
  m_listenfd=m_clientfd=0;
}
 
CTcpServer::~CTcpServer()
{
  if (m_listenfd!=0) close(m_listenfd);  // 析构函数关闭socket
  if (m_clientfd!=0) close(m_clientfd);  // 析构函数关闭socket
}
 
// 初始化服务端的socket，port为通信端口
bool CTcpServer::InitServer(int port)
{
  if (m_listenfd!=0) { close(m_listenfd); m_listenfd=0; }
 
  m_listenfd = socket(AF_INET,SOCK_STREAM,0);  // 创建服务端的socket
 
  // 把服务端用于通信的地址和端口绑定到socket上
  struct sockaddr_in servaddr;    // 服务端地址信息的数据结构
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family = AF_INET;  // 协议族，在socket编程中只能是AF_INET
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 本主机的任意ip地址
  servaddr.sin_port = htons(port);  // 绑定通信端口
  if (bind(m_listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) != 0 )
  { close(m_listenfd); m_listenfd=0; return false; }
 
  // 把socket设置为监听模式
  if (listen(m_listenfd,5) != 0 ) { close(m_listenfd); m_listenfd=0; return false; }
 
  return true;
}
 
bool CTcpServer::Accept()
{
  if ( (m_clientfd=accept(m_listenfd,0,0)) <= 0) return false;
 
  return true;
}
 
int CTcpServer::Send(const void *buf,const int buflen)
{
  return send(m_clientfd,buf,buflen,0);
}
 
int CTcpServer::Recv(void *buf,const int buflen)
{
  return recv(m_clientfd,buf,buflen,0);
}
 
// 与客户端通信线程的主函数
void *pth_main(void *arg)
{
  int clientfd=(long) arg; // arg参数为新客户端的socket。
 
  // 与客户端通信，接收客户端发过来的报文后，回复ok。
  char strbuffer[1024];
 
  while (1)
  {
    memset(strbuffer,0,sizeof(strbuffer));
    if (recv(clientfd,strbuffer,sizeof(strbuffer),0)<=0) break;
    printf("接收：%s\n",strbuffer);
 
    strcpy(strbuffer,"ok");
    if (send(clientfd,strbuffer,strlen(strbuffer),0)<=0) break;
    printf("发送：%s\n",strbuffer);
  }
 
  printf("客户端已断开连接。\n");
 
  close(clientfd);  // 关闭客户端的连接。
 
  pthread_exit(0);
}
```

​    需要注意几个问题：

​    1）线程主函数的函数体中，不能使用return;语句，如果想退出线程，可以用pthread_exit(0);返回。

​    2）线程可以共享全局变量，当然也可以共享TcpServer的m_clientfd成员变量，但是，创建线程的时候，为什么要把客户端的socket用参数传给线程主函数，而不是直接获取TcpServer.m_clientfd的值，因为主进程调用pthread_create创建线程后，立即返回循环重新Accept，创建线程需要时间，如果在这段时间内有新的客户端连接上来，TcpServer.m_clientfd的值会发生改变。

​    3）TcpServer.m_clientfd的强制转换，在创建线程的时候，代码如下：

```c++
if (pthread_create(&pthid,NULL,pth_main,(void*)((long)TcpServer.m_clientfd))!=0)
```

​    线程中的代码如下：

```c++
int clientfd=(long) arg; // arg参数为新客户端的socket。
```

​    这种数据类型的转换方法可能会让初学者不理解，在学习指针的时候说过，指针是用来存放变量的地址，不能把整数赋给指针，那现在这是怎么回事？这么说吧，C语言很灵活，数据类型可以强制转换，怎么转过去就怎么转回来。举个例子：水桶是用来装水的，特殊情况下用水桶来装板砖其实也可以，但是，板砖放入水桶的方法和从水桶中取出板砖的方法与水不同，怎么放进去就怎么取出来。

​    4）这个程序有一个漏洞，没有保存客户端的socket，主程序退出时，没有关闭客户端的socket，资源没有释放，这么说您可能难以理解，没有关系，等您真的需要编写多线程的socket服务端程序的时候就明白了。

#### 三、线程资源的回收

​    线程有joinable和unjoinable两种状态，如果线程是joinable状态，当线程主函数终止时（自己退出或调用pthread_exit退出）不会释放线程所占用内存资源和其它资源，这种线程被称为“僵尸线程”。创建线程时默认是非分离的，或者称为可连接的（joinable）。

​    避免僵尸线程就是如何正确的回收线程资源，有四种方法：

​    1）方法一：创建线程后，在创建线程的程序中调用pthread_join等待线程退出，一般不会采用这种方法，因为pthread_join会发生阻塞。

```
  pthread_join(pthid,NULL);
```

​    2）方法二：创建线程前，调用pthread_attr_setdetachstate将线程设为detached，这样线程退出时，系统自动回收线程资源。

```
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);  // 设置线程的属性。
  pthread_create(&pthid,&attr,pth_main,(void*)((long)TcpServer.m_clientfd);
```

​    3）方法三：创建线程后，在创建线程的程序中调用pthread_detach将新创建的线程设置为detached状态。

```
  pthread_detach(pthid);
```

​      4）方法四：在线程主函数中调用pthread_detach改变自己的状态。

```
  pthread_detach(pthread_self());
```

#### 四、查看线程

​    1）在top命令中，如果加上-H参数，top中的每一行显示的不是进程，而是一个线程。

```
top -H
```

​    2）在ps命令中加-xH参数也可以显示线程，加grep可以过滤内容。

```
ps -xH
ps -xH|grep book261
```

#### 五、应用经验

​    Linux没有真正意义上的线程，它的实现是由进程来模拟，属于用户级线程。所以，在Linux系统下，进程与线程在性能和资源消耗方面没有本质的差别。

​    **对我们程序员来说，进程不能共享全局数据，线程可以共享全局数据，各位可以根据应用场景选择采用多进程或多线程。**