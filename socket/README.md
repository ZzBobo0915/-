# socket网络通信

#### 一、网络通信socket

​    socket被称为套接字，运行在计算机中两个程序通过socket建立起一个通道，数据在通道中传输。

​    socket把复杂的TCP/IP协议隐藏在socket接口后面。

#### 二、套接字（socket）

​    TCP提供了流stream和数据包datagram两种通信机制，所以套接字也分为流套接字和数据报套接字：

1.  流套接字的类型是SOCK_STREAM，基于TCP协议，他提供一个有序、可靠、双向字节流的连接，确保发送的数据不会丢失、重复或乱序到达，而且后面还有出错后重发的机制。
2.  数据报套接字的类型是SOCK_DGRAM，基于UDP协议。不需要建立和维持连接，可能会丢失或错乱。UDP不是一个可靠的协议，对数据的长度有限制，但是它的速度比较高。

#### 三、客户/服务端模式

​    在TCP/IP网络应用中，两个程序之间通信模式是客户/服务端模式（client/server），客户/服务端也叫作客户/服务器，各人习惯。

##### 1.服务端的工作流程

​    1）创建服务端的socket。

​    2）把服务端用于通信的地址和端口绑定到socket上。

​    3）把socket设置为监听模式。

​    4）接受客户端的连接。

​    5）与客户端通信，接收客户端发过来的报文后，回复处理结果。

​    6）不断的重复第5）步，直到客户端断开连接。

​    7）关闭socket，释放资源。

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
int main(int argc,char *argv[])
{
    if (argc!=2)
    {
        printf("Using:./server port\nExample:./server 5005\n\n"); return -1;
    }
 
    // 第1步：创建服务端的socket。
    int listenfd;
    if ( (listenfd = socket(AF_INET,SOCK_STREAM,0))==-1) { perror("socket"); return -1; }
 
    // 第2步：把服务端用于通信的地址和端口绑定到socket上。
    struct sockaddr_in servaddr;    // 服务端地址信息的数据结构。
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;  // 协议族，在socket编程中只能是AF_INET。
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);          // 任意ip地址。
    //servaddr.sin_addr.s_addr = inet_addr("192.168.190.134"); // 指定ip地址。
    servaddr.sin_port = htons(atoi(argv[1]));  // 指定通信端口。
    if (bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) != 0 )
    { perror("bind"); close(listenfd); return -1; }
 
    // 第3步：把socket设置为监听模式。
    if (listen(listenfd,5) != 0 ) { perror("listen"); close(listenfd); return -1; }
 
    // 第4步：接受客户端的连接。
    int clientfd;                  // 客户端的socket。
    int socklen=sizeof(struct sockaddr_in); // struct sockaddr_in的大小
    struct sockaddr_in clientaddr;  // 客户端的地址信息。
    clientfd=accept(listenfd,(struct sockaddr *)&clientaddr,(socklen_t*)&socklen);
    printf("客户端（%s）已连接。\n",inet_ntoa(clientaddr.sin_addr));
 
    // 第5步：与客户端通信，接收客户端发过来的报文后，回复ok。
    char buffer[1024];
    while (1)
    {
      int iret;
      memset(buffer,0,sizeof(buffer));
      if ( (iret=recv(clientfd,buffer,sizeof(buffer),0))<=0) // 接收客户端的请求报文。
      {
        printf("iret=%d\n",iret); break;  
      }
      printf("接收：%s\n",buffer);
 
      strcpy(buffer,"ok");
      if ( (iret=send(clientfd,buffer,strlen(buffer),0))<=0) // 向客户端发送响应结果。
      { perror("send"); break; }
      printf("发送：%s\n",buffer);
    }
 
    // 第6步：关闭socket，释放资源。
    close(listenfd); close(clientfd);
}
```

##### 2.客户端的工作流程

​    1）创建客户端的socket。

​    2）向服务器发起连接请求。

​    3）与服务端通信，发送一个报文后等待回复，然后再发下一个报文。

​    4）不断的重复第3）步，直到全部的数据被发送完。

​    5）第4步：关闭socket，释放资源。

```c++
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
int main(int argc,char *argv[])
{
    if (argc!=3)
    {
        printf("Using:./client ip port\nExample:./client 192.168.235.131 5005\n\n"); return -1;
    }
 
    // 第1步：创建客户端的socket。
    int sockfd;
    if ( (sockfd = socket(AF_INET,SOCK_STREAM,0))==-1) { perror("socket"); return -1; }
 
    // 第2步：向服务器发起连接请求。
    struct hostent* h;
    if ( (h = gethostbyname(argv[1])) == 0 )   // 指定服务端的ip地址。
    { printf("gethostbyname failed.\n"); close(sockfd); return -1; }
    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2])); // 指定服务端的通信端口。
    memcpy(&servaddr.sin_addr,h->h_addr,h->h_length);
    if (connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)  // 向服务端发起连接清求。
    { perror("connect"); close(sockfd); return -1; }
 
    char buffer[1024];
 
    // 第3步：与服务端通信，发送一个报文后等待回复，然后再发下一个报文。
    for (int ii=0;ii<3;ii++)
    {
        int iret;
        memset(buffer,0,sizeof(buffer));
        sprintf(buffer,"这是第%d个超级女生，编号%03d。",ii+1,ii+1);
        if ( (iret=send(sockfd,buffer,strlen(buffer),0))<=0) // 向服务端发送请求报文。
        { perror("send"); break; }
        printf("发送：%s\n",buffer);
 
        memset(buffer,0,sizeof(buffer));
        if ( (iret=recv(sockfd,buffer,sizeof(buffer),0))<=0) // 接收服务端的回应报文。
        {
            printf("iret=%d\n",iret); break;
        }
        printf("接收：%s\n",buffer);
    }
 
    // 第4步：关闭socket，释放资源。
    close(sockfd);
}
```

​    先启动服务段程序server，服务端启动后，进入等待客户端连接状态，然后启动客户端：

```linux
./server 5005
./client 192.168.235.131 5005
```

#### 四、注意事项

##### 1.socket文件描述符

​    在LINUX系统中，一切输入输出设备皆文件，socket()函数的返回值其本质是一个文件描述符，是一个整数。

##### 2.服务端程序绑定地址

​    如果服务器有多个网卡，多个IP地址，socket通信可以指定用其中一个地址来进行通信，也可以任意ip地址：

```c++
m_servaddr.sin_addr.s_addr = inet_addr("192.168.149.129");  // 指定ip地址
m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 本主机的任意ip地址
```

##### 3.服务端程序绑定的通信端口

```c++
m_servaddr.sin_port = htons(5005);  // 通信端口
```

##### 4.客户端程序指定服务端的ip地址

```c++
struct hostent* h;
if ( (h = gethostbyname("118.89.50.198")) == 0 )   // 指定服务端的ip地址。
{ printf("gethostbyname failed.\n"); close(sockfd); return -1; }
```

##### 5.客户端程序指定服务端的通信端口

```c++
servaddr.sin_port = htons(5005);
```

##### 6.send函数

​    send函数用于把数据通过socket发送给对端。不论是客户端还是服务端，应用程序都用send函数来向TCP连接的另一端发送数据：

```c++
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

​    sockfd为已建立好连接的socket。

​    buf为需要发送的数据的内存地址，可以是C语言基本数据类型变量的地址，也可以数组、结构体、字符串，内存中有什么就发送什么。

​    len需要发送的数据的长度，为buf中有效数据的长度。

​    flags填0, 其他数值意义不大。

​    函数返回已发送的字符数。出错时返回-1，错误信息errno被标记。

​    注意，就算是网络断开，或socket已被对端关闭，send函数不会立即报错，要过几秒才会报错。如果send函数返回的错误（<=0），表示通信链路已不可用。

##### 7.recv函数

​    recv函数用于接收对端通过socket发送过来的数据。不论是客户端还是服务端，应用程序都用recv函数接收来自TCP连接的另一端发送过来数据：

```c++
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

​    sockfd为已建立好连接的socket。

​    buf为用于接收数据的内存地址，可以是C语言基本数据类型变量的地址，也可以数组、结构体、字符串，只要是一块内存就行了。

​    len需要接收数据的长度，不能超过buf的大小，否则内存溢出。

​    flags填0, 其他数值意义不大。

​    函数返回已接收的字符数。出错时返回-1，失败时不会设置errno的值。如果socket的对端没有发送数据，recv函数就会等待，如果对端发送了数据，函数返回接收到的字符数。出错时返回-1。如果socket被对端关闭，返回值为0。如果recv函数返回的错误（<=0），表示通信通道已不可用。

##### 8.服务端有两个socket

​    对服务端来说，有两个socket，一个是用于监听的socket，还有一个就是客户端连接成功后，由accept函数创建的用于与客户端收发报文的socket。

##### 9.程序退出时先关闭socket

​    socket是系统资源，操作系统打开的socket数量是有限的，在程序退出之前必须关闭已打开的socket，就像关闭文件指针一样，就像delete已分配的内存一样，极其重要。

​    值得注意的是，关闭socket的代码不能只在main函数的最后，那是程序运行的理想状态，还应该在main函数的每个return之前关闭。

#### 五、相关的库函数

#####   1.socket函数

  socket函数用于创建一个新的socket，也就是向系统申请一个socket资源。socket函数用户客户端和服务端：

```c++
int socket(int domain, int type, int protocol);
```

​    domain：协议域，又称协议族（family）。常用的协议族有AF_INET、AF_INET6、AF_LOCAL（或称AF_UNIX，Unix域Socket）、AF_ROUTE等。协议族决定了socket的地址类型，在通信中必须采用对应的地址，如AF_INET决定了要用ipv4地址（32位的）与端口号（16位的）的组合、AF_UNIX决定了要用一个绝对路径名作为地址。

​    type：指定socket类型。常用的socket类型有SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET等。流式socket（SOCK_STREAM）是一种面向连接的socket，针对于面向连接的TCP服务应用。数据报式socket（SOCK_DGRAM）是一种无连接的socket，对应于无连接的UDP服务应用。

​    protocol：指定协议。常用协议有IPPROTO_TCP、IPPROTO_UDP、IPPROTO_STCP、IPPROTO_TIPC等，分别对应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议。

​    一般第一个参数只能填AF_INET，第二个参数只能填SOCK_STREAM，第三个参数只能填0。除非系统资料耗尽，socket函数一般不会返回失败。

​    返回值：成功则返回一个socket，失败返回-1，错误原因存于errno 中。

##### 2.gethostbyname函数

​    把ip地址或域名转换为hostent 结构体表达的地址：

```c++
struct hostent *gethostbyname(const char *name);
```

​    参数name，域名或者主机名，例如"192.168.1.3"、"www.freecplus.net"等。

​    返回值：如果成功，返回一个hostent结构指针，失败返回NULL。

​    gethostbyname只用于客户端。gethostbyname只是把字符串的ip地址转换为结构体的ip地址，只要地址格式没错，一般不会返回错误。失败时不会设置errno的值。

##### 3.connect函数

​    向服务器发起连接请求：

```c++
int connect(int sockfd, struct sockaddr * serv_addr, int addrlen);
```

​    函数说明：connect函数用于将参数sockfd 的socket 连至参数serv_addr 指定的服务端，参数addrlen为sockaddr的结构长度。

​    返回值：成功则返回0，失败返回-1，错误原因存于errno 中。

​    connect函数只用于客户端。如果服务端的地址错了，或端口错了，或服务端没有启动，connect一定会失败。

##### 4.bind函数

​    服务端把用于通信的地址和端口绑定到socket上：

```c++
int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
```

​    参数sockfd，需要绑定的socket。

​    参数addr，存放了服务端用于通信的地址和端口。

​    参数addrlen表示addr结构体的大小。

​    返回值：成功则返回0，失败返回-1，错误原因存于errno 中。

​    如果绑定的地址错误，或**端口已被占用**，bind函数一定会报错，否则一般不会返回错误。

##### 5.listen函数

​    listen函数把主动连接socket变为被动连接的socket，使得这个socket可以接受其它socket的连接请求，从而成为一个服务端的socket：

```c++
int listen(int sockfd, int backlog);
```

​    参数sockfd是已经被bind过的socket。socket函数返回的socket是一个主动连接的socket，在服务端的编程中，程序员希望这个socket可以接受外来的连接请求，也就是被动等待客户端来连接。由于系统默认时认为一个socket是主动连接的，所以需要通过某种方式来告诉系统，程序员通过调用listen函数来完成这件事。

​    参数backlog，这个参数涉及到一些网络的细节，比较麻烦，填5、10都行，一般不超过30。

​    返回值：成功则返回0，失败返回-1，错误原因存于errno 中。

​    当调用listen之后，服务端的socket就可以调用accept来接受客户端的连接请求。

##### 6.accept函数

​    服务端接受客户端的连接：

```c++
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

​    参数sockfd是已经被listen过的socket。

​    参数addr用于存放客户端的地址信息，用sockaddr结构体表达，如果不需要客户端的地址，可以填0。

​    参数addrlen用于存放addr参数的长度，如果addr为0，addrlen也填0。

​    返回值：成功则返回0，失败返回-1，错误原因存于errno 中。

​    accept函数等待客户端的连接，如果没有客户端连上来，它就一直等待，这种方式称之为阻塞。accept等待到客户端的连接后，创建一个新的socket，函数返回值就是这个新的socket，服务端使用这个新的socket和客户端进行报文的收发。

​    accept在等待的过程中，如果被中断或其它的原因，函数返回-1，表示失败，如果失败，可以重新accept。

##### 7.过程小结

​    **服务端函数调用的流程是：socket->bind->listen->accept->recv/send->close**

​    **客户端函数调用的流程是：socket->connect->send/recv->close**

​    **其中send/recv可以进行多次交互。**

