# 封装socket

#### c类型

#### 1、封装客户端

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 把客户端连接服务端的socket操作封装到connecttoserver函数中
// TCP客户端连服务端的函数，serverip-服务端ip，port通信端口
int connecttoserver(const char *serverip,const int port);
 
int main()
{
    // 向服务器发起连接请求
    int sockfd=connecttoserver("192.168.235.131", 5005);
    //sockfd一般大于3，因为0、1、2为系统给定好的，不占用
    if (sockfd<=0) { printf("连接服务器失败，程序退出。\n"); return -1; }
 
    char strbuffer[1024];
 
    // 与服务端通信，发送一个报文后等待回复，然后再发下一个报文。
    for (int ii=0;ii<10;ii++)
    {
        memset(strbuffer,0,sizeof(strbuffer));
        sprintf(strbuffer,"这是第%d个超级女生，编号%03d。",ii+1,ii+1);
        if (send(sockfd,strbuffer,strlen(strbuffer),0)<=0) break;
        printf("发送：%s\n",strbuffer);
 
        memset(strbuffer,0,sizeof(strbuffer));
        if (recv(sockfd,strbuffer,sizeof(strbuffer),0)<=0) break;
        printf("接收：%s\n",strbuffer);
    }
 
    close(sockfd);
}
 
// TCP客户端连服务端的函数，serverip-服务端ip，port通信端口
// 返回值：成功返回已连接socket，失败返回-1。
int connecttoserver(const char *serverip,const int port)
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0); // 创建客户端的socket
 
    struct hostent* h; // ip地址信息的数据结构
    if ( (h = gethostbyname(serverip)) == 0 )
    { perror("gethostbyname"); close(sockfd); return -1; }
 
    // 把服务器的地址和端口转换为数据结构
    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    memcpy(&servaddr.sin_addr,h->h_addr,h->h_length);
 
    // 向服务器发起连接请求
    if (connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
    { perror("connect"); close(sockfd); return -1; }
 
    return sockfd;
}
```

#### 2、服务端

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 初始化服务端的socket，port为通信端口
// 返回值：成功返回初始化的socket值，否则返回-1
int initserver(int port);

int main()
{
    int listenfd = initserver(5005);  // 服务端用于监听的socket
    if (listenfd <= 0)
    { printf("服务端初始化失败，程序退出！\n"); return -1; }
    // 接受客户端的请求
    int clientfd;
    if ((clientfd = accept(listenfd, 0, 0)) <= 0)
    { printf("服务端accept失败，程序退出！\n"); return -1; }
    
    // 与客户端通信，接受客户端发来的报文后回复ok
    char strbuffer[1024];
    while (1)
    {
        memset(strbuffer, 0, sizeof(strbuffer));
        if (recv(clientfd, strbuffer, sizeof(strbuffer), 0) <= 0) break;
        printf("接受：%s\n", strbuffer);
        
        strcpy(strbuffer, "ok");
        if (send(clientfd, strbuffer, strlen(strbuffer), 0) <= 0) break;
        printf("发送:%s\n", strbuffer);
    }
    
    printf("客户端已断开连接。\n");
    close(clientfd); close(listenfd);  //关闭socket
}

//初始化服务端的socket，sort为通信端口
int initserver(int port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);  // 创建服务端的socket
    
    // 把服务端用于通信的地址和端口绑定到socket上
    struct sockaddr_in servaddr;  // 服务端地址信息的数据结构
    memeset(&servaddr, 0, sizeof(servaddr));
    sercaddr.sin_family = AF_INET;  //协议簇，在socket编程中只能是AF_INET
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 本主机的任意ip地址
    servaddr.sin_port = htons(port);  // 绑定通信端口
    if (bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) != 0 )
    { perror("bind"); close(listenfd); return -1; }
  
    // 把socket设置为监听模式
    if (listen(listenfd,5) != 0 ) { perror("listen"); close(listenfd); return -1; }
 
    return listenfd;
}

```



#### c++类型

#### 1、客户端

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
 
  for (int ii=0;ii<5;ii++)
  {
    memset(strbuffer,0,sizeof(strbuffer));
    sprintf(strbuffer,"这是第%d个超级女生，编号%03d。",ii+1,ii+1);
    if (TcpClient.Send(strbuffer,strlen(strbuffer))<=0) break;
    printf("发送：%s\n",strbuffer);
   
    memset(strbuffer,0,sizeof(strbuffer));
    if (TcpClient.Recv(strbuffer,sizeof(strbuffer))<=0) break;
    printf("接收：%s\n",strbuffer);
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

#### 2、服务端

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
 
 ~CTcpServer();
};
 
int main()
{
  CTcpServer TcpServer;
 
  if (TcpServer.InitServer(5005)==false)
  { printf("TcpServer.InitServer(5005) failed,exit...\n"); return -1; }
 
  if (TcpServer.Accept() == false) { printf("TcpServer.Accept() failed,exit...\n"); return -1; }
 
  printf("客户端已连接。\n");
 
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
```

