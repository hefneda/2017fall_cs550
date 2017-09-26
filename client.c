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
#define MAXLINE 512  

void c_client(void);
int lookup(int c_client_fd, char *filename);

char HOST[16];

int main(int argc, char** argv)  
{  

    if( argc != 2){  
        printf("usage: ./client <ipaddress>\n");  
        exit(0);  
    }  

    strcpy(HOST,argv[1]);
    c_client();                                                               //Handle this client as a client to receive file


    exit(0);  
}  

void c_client()
{
    int    c_client_fd;
    struct sockaddr_un    c_clientaddr;  
    char    recvline[MAXLINE], sendline[MAXLINE];  
    int    n,rec_len;  
    char    buf[MAXLINE]; 
    int cmdno=0;
    char    filename[MAXLINE], peerid[MAXLINE]; 
    char *end;
    int count=0;

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

    while(1)
    {
        //sendline=NULL;
        printf("Input the function No. : 1.Registry 2. Download File \n");  
        fgets(sendline, 4096, stdin);  
        printf("Imhere1\n");  
        //send cmd to central server and receive back
        if( send(c_client_fd, sendline, strlen(sendline), 0) < 0)  
        {  
            printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);  
            exit(0);  
        }  
        printf("Imhere2\n");  
        //receive confirm msg from central server
        if((rec_len = recv(c_client_fd, buf, MAXLINE,0)) == -1) {  
            perror("recv error");  
            exit(1);  
        }  
        printf("Imhere3\n");  
        cmdno=atoi(buf);
        printf("Received : %d\n ",cmdno);


//----------------------------------------------------Register
        if(cmdno == 1)
        {
            printf("Input the filename to register: ");  
            fgets(filename, MAXLINE, stdin);  

            if((end=strchr(filename,'\n')) != NULL)
			    *end = '\0';

            send(c_client_fd,(void *)filename,MAXLINE,0);
            send(c_client_fd,HOST,16,0);
        }
  //----------------------------------------------------Download
        if(cmdno == 2)
        {
            if(lookup(c_client_fd, filename)==1)           //found
            {
                printf("Ready to receive list\n");
                    //receive num of clients with files
                printf("Receiving numbers of lists\n");
                recv(c_client_fd, buf, MAXLINE,0);
                printf("------%s\n",buf);
                count=atoi(buf);
                printf("%d files have/has this file\n",count);
            }
            else
            {
                printf("In ");  
            }
        }
       
        if(cmdno == 3)
            break;
    }
    
    close(c_client_fd);  
}
int lookup(int c_client_fd, char *filename)
{
    int    n,rec_len;  
    char    buf[MAXLINE]; 
    char *end;

    printf("Input the filename to download: ");  
    fgets(filename, MAXLINE, stdin);  

    if((end=strchr(filename,'\n')) != NULL)
        *end = '\0';
    //send filename to download
    send(c_client_fd,(void *)filename,MAXLINE,0);
    //wait to see if cental server can find this file
    recv(c_client_fd, buf, MAXLINE,0);
    if(atoi(buf)==1)
    {
        printf("File found by server\n");  
        return 1;
    }
    else
    {
        printf("Fail to find file\n");  
        return 0;
    }
}