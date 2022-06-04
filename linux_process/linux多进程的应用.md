# linux多进程的引用

#### 一、并发的服务端

​    如果把socket服务端改为多进程，在每次accept到一个客户端的连接后，生成一个子进程，让子进程负责和这个客户端通信，父进程继续accept客户端的连接，socket的服务端在监听新客户端的同时，还可以与多个客户端进行通信。这就是并发。

##### 1.服务端

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
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
 
  void CloseClient();    // 关闭客户端的socket
  void CloseListen();    // 关闭用于监听的socket
 
 ~CTcpServer();
};
 
CTcpServer TcpServer;
 
int main()
{
  // signal(SIGCHLD,SIG_IGN);  // 忽略子进程退出的信号，避免产生僵尸进程
 
  if (TcpServer.InitServer(5051)==false)
  { printf("服务端初始化失败，程序退出。\n"); return -1; }
 
  while (1)
  {
    if (TcpServer.Accept() == false) continue;
 
    if (fork()>0) { TcpServer.CloseClient(); continue; }  // 父进程回到while，继续Accept。
 
    // 子进程负责与客户端进行通信，直到客户端断开连接。
    TcpServer.CloseListen();
 
    printf("客户端已连接。\n");
 
    // 与客户端通信，接收客户端发过来的报文后，回复ok。
    char strbuffer[1024];
 
    while (1)
    {
      memset(strbuffer,0,sizeof(strbuffer));
      if (TcpServer.Recv(strbuffer,sizeof(strbuffer))<=0) break;
      printf("接收：%s\n",strbuffer);
 
      strcpy(strbuffer,"ok");
      if (TcpServer.Send(strbuffer,strlen(strbuffer))<=0) break;
      printf("发送：%s\n",strbuffer);
    }
 
    printf("客户端已断开连接。\n");
 
    return 0;  // 或者exit(0)，子进程退出。
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
 
void CTcpServer::CloseClient()    // 关闭客户端的socket
{
  if (m_clientfd!=0) { close(m_clientfd); m_clientfd=0; }
}
 
void CTcpServer::CloseListen()    // 关闭用于监听的socket
{
  if (m_listenfd!=0) { close(m_listenfd); m_listenfd=0; }
}
```

1.  在CTcpServer中增加了两个成员函数

    ```c++
    void CloseClient();    // 关闭客户端的socket
    void CloseListen();    // 关闭用于监听的socket
    ```

2.  当有客户端连上来的时候，主进程执行fork，这时候会客户端的socket（m_clientfd）被复制了一份，对父进程来说，只负责监听客户端的连接，不需要与客户端通信，所以父进程关闭m_clientfd，注意，父进程关闭m_clientfd对子进程中的m_clientfd没有影响。

3.  当有客户端连上来的时候，主进程执行fork，这时候服务端用于监听的socket（m_listenfd）也会被复制了一份，对子进程来说，只需要与客户端通信，不需要监听客户端的连接，所以子进程关闭监听的m_listenfd，同理，子进程关闭m_listenfd对父进程中的m_listenfd没有影响。

4.  子进程执行完任务后，要调用retrun或exit(0)退出，如果没有调用return或exit(0)，子进程将又会回到while循环首部。

##### 2.客户端

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
// TCP客户端类
class CTcpClient
{
public:
  int m_sockfd;
 
  CTcpClient();
 
  // 向服务器发起连接，serverip-服务端ip，port通信端口
  bool ConnectToServer(const char *serverip,const int port);
  // 向对端发送报文
  int  Send(const void *buf,const int buflen);
  // 接收对端的报文
  int  Recv(void *buf,const int buflen);
 
 ~CTcpClient();
};
 
int main()
{
  CTcpClient TcpClient;
 
  // 向服务器发起连接请求
  if (TcpClient.ConnectToServer("192.168.235.131",5005)==false)
  { printf("TcpClient.ConnectToServer(\"192.168.235.131\",5005) failed,exit...\n"); return -1; }
 
  char strbuffer[1024];
 
  for (int ii=0;ii<50;ii++)
  {
    memset(strbuffer,0,sizeof(strbuffer));
    sprintf(strbuffer,"这是第%d个超级女生，编号%03d。",ii+1,ii+1);
    if (TcpClient.Send(strbuffer,strlen(strbuffer))<=0) break;
    printf("发送：%s\n",strbuffer);
   
    memset(strbuffer,0,sizeof(strbuffer));
    if (TcpClient.Recv(strbuffer,sizeof(strbuffer))<=0) break;
    printf("接收：%s\n",strbuffer);
 
    sleep(1);  // sleep一秒，方便观察程序的运行。
  }
}
 
CTcpClient::CTcpClient()
{
  m_sockfd=0;  // 构造函数初始化m_sockfd
}
 
CTcpClient::~CTcpClient()
{
  if (m_sockfd!=0) close(m_sockfd);  // 析构函数关闭m_sockfd
}
 
// 向服务器发起连接，serverip-服务端ip，port通信端口
bool CTcpClient::ConnectToServer(const char *serverip,const int port)
{
  m_sockfd = socket(AF_INET,SOCK_STREAM,0); // 创建客户端的socket
 
  struct hostent* h; // ip地址信息的数据结构
  if ( (h=gethostbyname(serverip))==0 )
  { close(m_sockfd); m_sockfd=0; return false; }
 
  // 把服务器的地址和端口转换为数据结构
  struct sockaddr_in servaddr;
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  memcpy(&servaddr.sin_addr,h->h_addr,h->h_length);
 
  // 向服务器发起连接请求
  if (connect(m_sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))!=0)
  { close(m_sockfd); m_sockfd=0; return false; }
 
  return true;
}
 
int CTcpClient::Send(const void *buf,const int buflen)
{
  return send(m_sockfd,buf,buflen,0);
}
 
int CTcpClient::Recv(void *buf,const int buflen)
{
  return recv(m_sockfd,buf,buflen,0);
}
```

#### 二、僵尸进程

##### 1.僵尸进程产生的原因

​    一个子进程在调用return或exit(0)结束自己的生命的时候，其实它并没有真正的被销毁，而是留下一个僵尸进程。僵尸进程有<defunct>标志。

##### 2.僵尸进程的危害

​    僵尸进程是子进程结束时，父进程又没有回收子进程占用的资源。

​    僵尸进程在消失之前会继续占用系统资源。

​    如果父进程先退出，子进程被系统接管，子进程退出后系统会回收其占用的相关资源，不会成为僵尸进程。父进和先退出的应用场景在以后的章节中介绍。

##### 3.如何解决僵尸进程

​    解决僵尸进程的方法有两种：

1.  子进程退出之前，会向父进程发送一个信号，父进程调用wait函数等待这个信号，只要等到了，就不会产生僵尸进程。这话说得容易，在并发的服务程序中这是不可能的，因为父进程要做其它的事，例如等待客户端的新连接，不可能去等待子进程的退出信号，这个方法我就不介绍了。

2.  另一种方法就是父进程直接忽略子进程的退出信号，具体做法很简单，在主程序中启用以下代码：

    ```c++
    signal(SIGCHLD,SIG_IGN);  // 忽略子进程退出的信号，避免产生僵尸进程
    ```

#### 三、应用经验

​    在学习了多进程的基础知识之后，初学者可能会认为多进程是一个高大上的技术，认为多进程处理数据肯定比单进程快，其实不是。在实际开发中，采用多进程的主要目的是处理多个并发的任务，而不是为了提高程序的效率。

​    从效率方面来说，某些场景下多进程的效率比单进程低，原因很简单，因为在有限的硬件资源中，多进程程序的内存开销更大，还会产生资源的竞争。就像多个人端着一盆水，不如一个人端着一盆水走得快。



