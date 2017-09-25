/* File Name: server.c */  
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

void ddd(void);

#define NUM_C 3


int socket_fd;

struct sockaddr_un     servaddr; 

int main(int argc, char** argv)  
{  
   //setup server by using sockets
    int i;
    int flag=666;
    pthread_t thread[NUM_C *3];
    
    if( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){                            //initial Socket  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  

    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sun_family = AF_UNIX;  
    strcpy(servaddr.sun_path, "SERV");
    unlink(servaddr.sun_path);

  
    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){          //bind
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }

    
    if( listen(socket_fd, NUM_C * 2) == -1){  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);                 //listen
        exit(0);  
    }  
    printf("======waiting for client's request======\n");  
    
    //create threads


    for(i = 0; i < NUM_C*2; i++)
        flag=pthread_create(&thread[i],NULL,(void *)ddd,NULL);

    printf("%d\n",flag);  
    close(socket_fd);  
    return 0;
}  

void ddd(void)
{

    printf("ddd");
    pthread_exit(0);
    return NULL;
}