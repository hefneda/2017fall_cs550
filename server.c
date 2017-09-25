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

typedef struct
{
    char *filename;
	char peerid[16];
	
} pfile;


void fthread(void);
int registry(const char *peerid, const char *filename);
int check_file(const char *peerid, const char *filename);
void get_filelist();

#define NUM_C 3
#define MAXLINE 512
#define MAXFILENUM 99



int socket_fd;
struct sockaddr_un     servaddr; 
pfile *files[MAXFILENUM] = {NULL};                //filelist in central server

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
    int cmdno=0;
    socklen_t len = sizeof(c_address);
    char cmdstr[2];                               //1:registry 2:Search File
    char filename[MAXLINE];
    char peerid[16];

    printf("Begin Accept! \n");             //accept clients
    if( (c_fd = accept(socket_fd, (struct sockaddr*)&c_address, &len)) == -1)
    {  
        printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
    }  

    printf("Client Connected, Wait client cmd \n");

    while(1)
    {  
        printf("Wait another client cmd \n");

        if(recv(c_fd,(void *)cmdstr,2,0) == 0)
            break;

        int cmdno= atoi(cmdstr);
        switch(cmdno)
        {
        case 1:                                          //For registry
            if(send(c_fd, "1", 8,0) == -1)    //send confirm msg to client
                perror("send error");
            printf("Request for Registry Rceived, Begin to Receive Filename and peerid\n");
            recv(c_fd,(void *)filename,MAXLINE,0);
            recv(c_fd,(void *)peerid,16,0);

            printf("Registry with filename: %s; Peerid:%s \n",filename,peerid);

            //Register the file 
           registry(peerid,filename);

           printf("Register Success!\n");
           printf("The PID of this process is %d\n",getpid());

          
           get_filelist();
           break;

        case 2:
            printf("Request for Download Received\n");
                
            break;

        }

        ////Send back  
        //if(!fork())
        //{ 
        //    if(send(c_fd, "1", 26,0) == -1)  
        //        perror("send error");    
        //}  
    }
    close(c_fd);
}

//register file
int registry(const char *peerid, const char *filename)
{
    int i;
    if(check_file(peerid,filename)==1)    ///check if the filename with the peerid has already been registrated
    {
        printf("File already registered\n");
        return -1;
    }

    for(i = 0; i < MAXFILENUM; i++)
    {
        if(files[i] == NULL)
        {
            files[i] = malloc(sizeof(pfile));
            strcpy(files[i]->peerid,peerid);
            files[i]->filename = malloc(sizeof(*filename)); 
            strcpy(files[i]->filename,filename);
            return 0;
            
        }
    }
    printf("No more files!\n");
    return -1;
}

//check if the filename with the peerid has already been registrated
int check_file(const char *peerid, const char *filename)
{
    int i;
	for(i = 0; i < MAXFILENUM; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0 && strcmp(files[i]->peerid,peerid) == 0)
			return 1;
	}
    return 0;
}

void get_filelist()
{
    int i=0;
    if(files[0] =NULL)
    {
        printf("Empty Filelist!\n");
        return;
    }
    printf("%d\n",i);
    printf("get:%s,%s\n",files[i]->filename,files[i]->peerid); 
    printf("-----------------------\n");  
    for(i = 0; i < MAXFILENUM; i++)
	{
        if(files[i] != NULL)
        {
            printf("%s,%s\n",files[i]->filename,files[i]->peerid);       
        }
        else
        {
            printf("-----------------------\n");  
            break;
        }
	}
}