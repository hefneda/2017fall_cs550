/* File Name: client.c */  

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
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


void th_func_c(void);
void th_func_s(void);
void c_client(void);
void c_server(void);
int lookup(int c_client_fd, char *filename);
void build_serversock(void);                                
void create_th(void);  
void c_server(void);


#define NUM_C 3
#define MAXLINE 512
#define MAXFILENUM 99
#define BUFF_SIZE (10*1024)

char HOST[16];
struct sockaddr_un csaddr;
int cs_fd;


int main(int argc, char** argv)  
{  

    if( argc != 2){  
        printf("usage: ./client <ipaddress>\n");  
        exit(0);  
    }  

    strcpy(HOST,argv[1]);
    //build_serversock();                                                //use 1 thread to build a client as a server
    create_th();
    //c_client();                                                               //Handle this client as a client to receive file


    exit(0);  
}  
void create_th(void)
{
    //Create the n threads
	pthread_t threads[NUM_C+1];                           //num of clients and indexing server
	int i;

	for(i = 0; i < NUM_C+1; i++)
	{
		//Create threads, and send their index in num using p
        if(i==0)
		    pthread_create(&threads[i],NULL,(void *)th_func_c,NULL);//just 1 thread to receive file or registry
        else
            pthread_create(&threads[i],NULL,(void *)th_func_s,NULL);//default threads as to send file

	}
	/* Terminate all threads */
	for(i = 0; i < NUM_C+1; i++)
	{
		pthread_join(threads[i],NULL);
	}
}
void th_func_c(void)
{
			//run this client as a client to receive and lookup file and registry
			c_client();

}
void th_func_s(void)
{

			//run this client as a server to send files
			c_server();

}
//------run the client as a server to send files to other clients
void build_serversock(void)
{
    //Create client server
	cs_fd = socket(AF_UNIX,SOCK_STREAM,0);
	if(cs_fd < 0)
	{
		perror("Socket");
		exit(0);
	}

	//Set socket structure vars
	memset(&csaddr,0,sizeof(csaddr));
	csaddr.sun_family = AF_UNIX;
	//strcpy(csaddr.sun_path,HOST);
	unlink(csaddr.sun_path);

	int err;
	err = bind(cs_fd,(struct sockaddr*)&csaddr,sizeof(csaddr));	
	if(err < 0)
	{
		perror("Bind");
		exit(0);
	}
	//Socket over localhost
	//Listen for connections, maximum of 1 client (1 per thread)
	err = listen(cs_fd,NUM_C-1);
	if(err < 0)
	{
		perror("Listen");
		exit(0);
	}
}
//---------------------run this client as a server to send files
void c_server(void)
{
    struct sockaddr_un ccaddr;
    int cc_fd;
    int file_d;
    char filename[MAXLINE];
    struct stat filestat;

    socklen_t len = sizeof(ccaddr);
    //Loop
    while(1)
    {
        printf("Client server wait for Client client connect\n");
        cc_fd = accept(cs_fd,(struct sockaddr*)&ccaddr,&len);    //cc_fd is the fd of receive client
        if(cc_fd < 0)
        {
            close(cc_fd);
            return;
        }
        printf("Connect success\n");

        
        //Receive the filename, if no clients to accept, return
        if(recv(cc_fd,(void *)filename,MAXLINE,0) == 0)
        {
            close(cc_fd);
            return;
        }
        printf("Send file: %s\n",filename);

        //Send file
        printf("File would be sent here\n");
        //Open the file
        file_d = open(filename,O_RDONLY); 
        if(file_d < 0)
        {
            printf("Fail to open file\n");
            close(cc_fd);
            return;
        }
        
        fstat(file_d,&filestat);
        char filesize[MAXLINE];
        //transmit fstat filesize to string
        sprintf(filesize, "%d",(int)filestat.st_size);
        //Send the file size
        send(cc_fd,(void *)filesize,MAXLINE,0);
        off_t len = 0;
        //Send the entire file
        if(sendfile(file_d,cc_fd,&len,BUFF_SIZE) < 0)
        {
            printf("Error sending file\n");
            close(cc_fd);
            return;
        }
        printf("File sent\n");
        //close file 
        close(file_d);
        //close client connection
        close(cc_fd);
    }
}

//---------------------run this client as a client to download file, lookup file and registry
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
                //printf("Ready to receive list\n");
                //    //receive num of clients with files
                //printf("Receiving numbers of lists\n");
                //recv(c_client_fd, buf, MAXLINE,0);
                //printf("------%s\n",buf);
                //count=atoi(buf);
                //printf("%d files have/has this file\n",count);

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
    char    str[MAXLINE];

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
        //recv(c_client_fd, str,16,0);
        //send(c_client_fd,"tty",16,0);

        if((rec_len = recv(c_client_fd,str, MAXLINE,0)) == -1) {  
            perror("recv error");  
            exit(1);  
        }  
        printf("------%s\n",str);
        return 1;
    }
    else
    {
        printf("Fail to find file\n");  
        return 0;
    }
}
