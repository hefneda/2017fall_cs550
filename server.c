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

void fthread(void);

#define NUM_C 3
#define MAX 128


int socket_fd;

struct sockaddr_un     servaddr; 

int main(int argc, char** argv)  
{  
   //setup server by using sockets
    int i;
    pthread_t thread[NUM_C *3];
    
    if( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){                            //initial Socket  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  

    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sun_family = AF_UNIX;  
    //strcpy(servaddr.sun_path, "SERV");
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
    
    //create threads to handle multiple tasks

    for(i = 0; i < NUM_C * 2; i++)
        pthread_create(&thread[i],NULL,(void *)fthread,NULL);
    for(i = 0; i < NUM_C * 2; i++)
		pthread_join(thread[i],NULL);

    close(socket_fd);  
    return 0;
}  

void fthread(void)                               //wait for registry client
{
    struct sockaddr_un c_address;       //registry client address
    int c_fd;                                           //registry client fd
    socklen_t len = sizeof(c_address);
    char cmdstr[2];                               //1:registry 2:Search File
    char filename[MAX];

    printf("Begin Accept! \n");
    while(1)
    {  
        if( (c_fd = accept(socket_fd, (struct sockaddr*)&c_address, &len)) == -1)
        {  
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
            continue;  
        }  

        printf("Registry Client Connected, Waiting for Receive\n");
        if(recv(c_fd,(void *)cmdstr,2,0) == 0)
            break;
        printf("RCEIVED:%s",cmdstr);
        //Send back  
        if(!fork())
        { 
            if(send(c_fd, "Message Received\n", 26,0) == -1)  
                perror("send error");    
        }  
        break;
    }
    close(c_fd);
}