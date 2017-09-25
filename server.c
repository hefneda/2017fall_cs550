/* File Name: server.c */  
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<pthread.h>
#define DEFAULT_PORT 8000  
#define MAXLINE 4096  

int setup_server();

int socket_fd;
struct sockaddr_un     servaddr; 

int main(int argc, char** argv)  
{  
    setup_server();
    
    close(socket_fd);  
}  

int setup_sever()
{
    char    buff[4096];  
    int     n;  

    //初始化Socket  
    if( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  

    //初始化  
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sun_family = AF_UNIX;  
    servaddr.sun_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
    servaddr.sun_port = htons(DEFAULT_PORT);//设置的端口为DEFAULT_PORT  

    //将本地地址绑定到所创建的套接字上  
    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }
        if( listen(socket_fd, NUM_C * 2) == -1){  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  
     printf("======waiting for client's request======\n");  
    return 0;
}