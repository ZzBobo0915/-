#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>

void fatchfunc()  // 父进程流程的主函数
{
    printf("我是老子，我喜欢孩子他娘。\n");
}
 
void childfunc()  // 子进程流程的主函数
{
    printf("我是儿子，我喜欢西施。\n");
}
 
int main()
{
    if (fork()>0) { printf("这是父进程，将调用fatchfunc()。\n"); fatchfunc();}
    else { printf("这是子进程，将调用childfunc()。\n");  childfunc();}
 
    sleep(1); printf("父子进程执行完自己的函数后都来这里。\n"); sleep(1);
} 
