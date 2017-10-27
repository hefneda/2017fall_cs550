 
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


void fthread(void* socket);
int registry(const char *peerid, const char *filename);
int check_file(const char *peerid, const char *filename);
void print_registry(void);
int search(char *filename );
void sendidlist(int c_fd,char* filename);
void build(int z);
void th_func(void *i);

#define NUM_C 4
#define MAXLINE 512
#define MAXFILENUM 99
#define NUM_S 2

char HOST[4][16]={"SERV1","SERV2","SERV3","SERV4"};
//int socket_fd;
//struct sockaddr_un     servaddr; 
pfile *files[MAXFILENUM] = {NULL};                //filelist in central server

int main(int argc, char** argv)  
{  

    pthread_t threads[ NUM_S];                           //num of clients and indexing server
	int i;
    int num[ NUM_S] = {0};
	int *p = num;

    for(i = 0; i <  NUM_S; i++)
	{
		num[i] = i;
		//Create threads, and send their index in num using p
		pthread_create(&threads[i],NULL,(void *)th_func,p);
		p++;
	}
    for(i = 0; i < NUM_S; i++)
    {
    	pthread_join(threads[i],NULL);
    }
   
    return 0;
}  

void th_func(void *i)
{
    //run this client as a client to receive and lookup file and registry
    int num = *((int *)i);
    printf("------------This is index server%d--------\n",num+1); 
    build(num);

}

void build(int z)
{
    int i;
    pthread_t thread[NUM_C];
    int on=1;  
    int ret;

    int socket_fd;
    struct sockaddr_un     servaddr; 

     if( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){                            //initial Socket  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  

    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sun_family = AF_UNIX;  
    strcpy(servaddr.sun_path, HOST[z]);
    unlink(servaddr.sun_path);
    //------------------------------avoid error: address already in use

    ret=setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));


    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){          //bind
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }
    printf("bind socket success, address:%s\n",servaddr.sun_path);

    if( listen(socket_fd, NUM_C ) == -1){  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);                 //listen
        exit(0);  
    }  
    printf("======waiting for client's request%d======\n",socket_fd);  

    //create threads to handle multiple tasks

    for(i = 0; i < NUM_C ; i++)
        pthread_create(&thread[i],NULL,(void *)fthread,&socket_fd);
    for(i = 0; i < NUM_C ; i++)
        pthread_join(thread[i],NULL);

    close(socket_fd);  
}
void fthread(void *socket)                               //wait for registry client
{
    struct sockaddr_un c_address;       //registry client address
    int c_fd;                                           //registry client fd
    int cmdno=0;
    
    char cmdstr[2];                               //1:registry 2:Search File
    char filename[MAXLINE];
    char peerid[16];
    pfile *found_files[MAXFILENUM] = {NULL}; 

    int socket_fd= *((int *)socket);//---------------------------------------------------------------------------------------------------------
    printf("Begin%d \n:",socket_fd);  
    //printf("%s Begin accept--\n",servaddr.sun_path);  
    while(1)
    {  
        socklen_t len = sizeof(c_address);
        printf("Begin accept:");  
        if( (c_fd = accept(socket_fd, (struct sockaddr*)&c_address, &len)) == -1)
        {  
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
        }  

        printf("Client Connected, Wait client cmd \n");

        if(recv(c_fd,(void *)cmdstr,2,0) == 0)
            break;

        int cmdno= atoi(cmdstr);
        switch(cmdno)
        {
//-------------------------------------------------------------For registry
        case 1:                                                                    
            if(send(c_fd, "1", 8,0) == -1)                              //send confirm msg to client
                perror("send error");
            printf("Request for Registry Rceived, Begin to Receive Filename and peerid\n");
            recv(c_fd,(void *)filename,MAXLINE,0);
            recv(c_fd,(void *)peerid,16,0);

            printf("Registry with filename: %s; Peerid:%s \n",filename,peerid);

            //Register the file 
           registry(peerid,filename);

           printf("Register Success!\n");

           //print filelist
          print_registry();
           break;
//-------------------------------------------------------------For searchfile
        case 2:
            if(send(c_fd, "2", 8,0) == -1)                              //send confirm msg to client
                perror("send error");
            printf("Request for Download Received\n");
            recv(c_fd,(void *)filename,MAXLINE,0);              //receive filename
            
            if(search(filename)!=0)                   //filename found
            {
                printf("We found it\n");
                //send back confimation
                send(c_fd, "1", 8,0);        
                usleep(1000);
                //send back the peerids with this filename
                sendidlist(c_fd,filename);

            }
            else                                                                     //filename not found
            {
                printf("We can not find it\n");
                send(c_fd, "2", 8,0);
            }
            
            break;
        }

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

//check if the filename with the peerid has alreadyFF been registrated
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

void print_registry()
{
	int i;
	for(i = 0; i < MAXFILENUM; i++)
	{
		if(files[i] != NULL)
		{
			printf("File %d : %s | Client : %s\n",i,files[i]->filename,files[i]->peerid);
		}
	}
}

int search(char *filename )
{
   int i;
	for(i = 0; i < MAXFILENUM; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)
			return 1;
	}
	return 0;

}

void sendidlist(int c_fd, char* filename)
{
    int i;
    int count = 0;
    char str[2];
    
    for(i = 0; i < MAXFILENUM; i++)
    {
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)
		{
			count++;
		}
	}
    //transmit int to string to send

    snprintf(str,sizeof(str),"%d",count);
    printf("Found %s clients with file\n",str);

    send(c_fd, str, MAXLINE,0);// send how many peers have file

    //Send clients who has file to request client
    for(i = 0; i < MAXFILENUM; i++)
	{
		if(files[i] != NULL && strcmp(files[i]->filename,filename) == 0)		
		{
			printf("Sending peerlist %d: %s\n",i,files[i]->peerid);
			send(c_fd,(files[i]->peerid),16,0);
		}
	}
}

