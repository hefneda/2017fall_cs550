/* File Name: client.c */  

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>
#define MAXLINE 4096  

void c_client(void);

int main(int argc, char** argv)  
{  

    if( argc != 2){  
        printf("usage: ./client <ipaddress>\n");  
        exit(0);  
    }  
    c_client();                                                               //Handle this client as a client to receive file


    exit(0);  
}  

void c_client()
{
    int    c_client_fd;
    struct sockaddr_un    c_clientaddr;  
    char    recvline[4096], sendline[4096];  
    int    n,rec_len;  
    char    buf[MAXLINE];  

    if( (c_client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){  
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);  
        exit(0);  
    }  
    printf("Success Create Socket \n");  

    memset(&c_clientaddr, 0, sizeof(c_clientaddr));  
    c_clientaddr.sun_family = AF_UNIX;  
    //strcpy(c_clientaddr.sun_path, "c_client");

    printf("Begin connect \n");  
    if( connect(c_client_fd, (struct sockaddr*)&c_clientaddr, sizeof(c_clientaddr)) < 0){  
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  


    printf("Input the function No. : 1.Registry 2. Download File \n");  
    fgets(sendline, 4096, stdin);  
    if( send(c_client_fd, sendline, strlen(sendline), 0) < 0)  
    {  
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);  
        exit(0);  
    }  
    if((rec_len = recv(c_client_fd, buf, MAXLINE,0)) == -1) {  
        perror("recv error");  
        exit(1);  
    }  
    buf[rec_len]  = '\0';  
    printf("Received : %s ",buf);  
    close(c_client_fd);  
}