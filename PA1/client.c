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
#include <errno.h>


void th_func(void *i);
void c_client(void);
void c_server(void);
int lookup(int c_client_fd, char *filename);
void build_serversock(void);                                
void create_th(void);  
void c_server(void);
void download(char *filename,char *peerid);

#define NUM_C 4
#define MAXLINE 512
#define MAXFILENUM 99
#define MAXFILESIZE 512
#define BUFF_SIZE (10*1024)

char HOST[16];
struct sockaddr_un csaddr;
int cs_fd;
int f=0;
FILE *file_out;

int main(int argc, char** argv)  
{  
    extern int errno;  
  
    errno=0;  
    if( argc != 2)
    {  
        printf("usage: ./client <ipaddress>\n");  
        exit(0);  
    }  
    strcpy(HOST,argv[1]);
    build_serversock();                                                //use 1 thread to build a client as a server
    create_th();
    //c_client();                                                               //Handle this client as a client to receive file


    exit(0);  
}  
void create_th(void)
{
    //Create the n threads
	pthread_t threads[NUM_C+1];                           //num of clients and indexing server
	int i;
    int num[NUM_C] = {0};
	int *p = num;

    for(i = 0; i < NUM_C; i++)
	{
		num[i] = i;
		//Create threads, and send their index in num using p
		pthread_create(&threads[i],NULL,(void *)th_func,p);
		p++;
	}
	for(i = 0; i < NUM_C+1; i++)
	{
		pthread_join(threads[i],NULL);
	}
}
void th_func(void *i)
{
    //run this client as a client to receive and lookup file and registry
    int num = *((int *)i);
    switch(num)
    {
    case 0:
        //just 1 thread to receive file or registry
        printf("------------This is a client thread--------\n"); 
        c_client();
        break;
    default:
        //default threads as to send file
        printf("------------This is server thread%d--------\n",num); 
        c_server();
        break;
    }

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
	strcpy(csaddr.sun_path,HOST);
	unlink(csaddr.sun_path);

    //------------------------------avoid error: address already in use
    int on=1;  
    int ret;
    ret=setsockopt(cs_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

	int err;
	err = bind(cs_fd,(struct sockaddr*)&csaddr,sizeof(csaddr));	
	if(err < 0)
	{
		perror("Bind");
		exit(0);
	}
    printf("bind socket success, Socket file of this thread:%s\n",csaddr.sun_path);
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
   
    printf("I am waiting for a client to connect and begin download\n");
    while(1)
    {
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
        printf("Begin open file\n");
        //Open the file
        file_d = open(filename,O_RDONLY); 

        if(file_d < 0)
        {
            perror("Fail to open file\n");
            close(cc_fd);
            return;
        }

        fstat(file_d,&filestat);
        char filesize[MAXLINE];
        //transmit fstat filesize to string
        sprintf(filesize, "%d",(int)filestat.st_size);

        printf("Filesize:%s\n",filesize);
        //Send the file size
        send(cc_fd,(void *)filesize,MAXLINE,0);
        off_t len = 0;
        //Send the entire file
         
        if( sendfile(cc_fd,file_d,&len,BUFF_SIZE)< 0)
        {
            perror("Error sending file");
            close(cc_fd);
            return;
        }
        printf("File sent, I will continue waiting for a Client to connect\n");
        //close file 
        close(file_d);
        //close client connection
        close(cc_fd);
        printf("Choose : 1.Registry 2. Download File 3.Quit \n");  // client thread is still running, user should communicate with that thread, instead of server thread
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
    char    msg[MAXLINE];
    struct timeval etstart, etstop;  /* Elapsed times using gettimeofday() */
	struct timezone tzdummy;
	clock_t etstart2, etstop2;	/* Elapsed times using times() */
	unsigned long long usecstart, usecstop;
	struct tms cputstart, cputstop;  /* CPU times for my processes */

    if( (c_client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){  
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);  
        exit(0);  
    }  
    printf("Success Create Socket \n");  

    memset(&c_clientaddr, 0, sizeof(c_clientaddr));  
    c_clientaddr.sun_family = AF_UNIX;  
    strcpy(c_clientaddr.sun_path, "../SERV");

    printf("Begin connect \n");  
    if( connect(c_client_fd, (struct sockaddr*)&c_clientaddr, sizeof(c_clientaddr)) < 0){  
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  
    

    while(1)
    {
        //sendline=NULL;
        printf("Choose : 1.Registry 2. Download File 3.Quit \n");  
        fgets(sendline, 4096, stdin);  
        //send cmd to central server and receive back
        if( send(c_client_fd, sendline, strlen(sendline), 0) < 0)  
        {  
            printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);  
            exit(0);  
        }   
        //receive confirm msg from central server
        if((rec_len = recv(c_client_fd, buf, MAXLINE,0)) == -1) {  
            perror("recv error");  
            exit(1);  
        }  
        cmdno=atoi(buf);
        printf("Received : %d\n ",cmdno);


//----------------------------------------------------Register
        if(cmdno == 1)
        {
            printf("Input the filename to register: ");  
            fgets(filename, MAXLINE, stdin);  

            if((end=strchr(filename,'\n')) != NULL)
                *end = '\0';
            //display in output
            file_out = fopen("../output.txt","a+");
            sprintf(msg,"%s register filename %s\n",HOST,filename);
            fwrite(msg,1,strlen(msg),file_out);
            fclose(file_out);
            send(c_client_fd,(void *)filename,MAXLINE,0);
            send(c_client_fd,HOST,16,0);
        }
  //----------------------------------------------------Download
        if(cmdno == 2)
        {
                printf("Input the filename to download: ");  
                fgets(filename, MAXLINE, stdin);  
                if((end=strchr(filename,'\n')) != NULL)
                    *end = '\0';
                 //display in output
               file_out = fopen("../output.txt","a+");
                sprintf(msg,"%s search filename %s to download\n",HOST,filename);
                fwrite(msg,1,strlen(msg),file_out);
                 fclose(file_out);
                //Start clock--------------------------------------------------------------------------------------
                gettimeofday(&etstart, &tzdummy);
                etstart2 = times(&cputstart);
                printf("time test begin!\n");

                lookup(c_client_fd,filename);   //find file and download

                printf("time test over!\n");
                //stop clock
                gettimeofday(&etstop, &tzdummy);
                etstop2 = times(&cputstop);
                usecstart = (unsigned long long)etstart.tv_sec * 1000000 + etstart.tv_usec;
                usecstop = (unsigned long long)etstop.tv_sec * 1000000 + etstop.tv_usec;
                //display time result
                printf("\nAvg Response time = %g ms.\n",(float)(usecstop - usecstart)/(float)1000);
        }
    //----------------------------------------------------quit     
        if(cmdno == 3)
            break;
    }
    
    close(c_client_fd);  
}
int lookup(int c_client_fd, char *filename)
{
    int    n,rec_len,peernum;  
    char    buf[MAXLINE]; 
    char *end;
    char    str[MAXLINE];
    char    msg[MAXLINE];
    char peerlist[NUM_C][16];
    char peerid[16];
    int count=0;
    int i=0;
   file_out = fopen("../output.txt","a+");
    //send filename to download
    send(c_client_fd,(void *)filename,MAXLINE,0);
    //wait to see if cental server can find this file

    recv(c_client_fd, buf, 8,0);
    if(atoi(buf)==1)
    {
        printf("File found by server\n");  

        recv(c_client_fd, str, MAXLINE,0);// receive how many peers have file
        count = atoi(str);
        printf("%d clients have file, ready to receive peerid list\n",count);
       
        //receive peerid list
        sprintf(msg," file found in index server,list as below:\n");
        fwrite(msg,1,strlen(msg),file_out);

       for(i = 0; i < count; i++)
		{
			recv(c_client_fd,(void *)&peerlist[i][0],16,0);	
			printf("%d: %s\n",i,peerlist[i]);
            //display in output
             sprintf(msg,"  %d: %s\n",i,peerlist[i]);
            fwrite(msg,1,strlen(msg),file_out);
		}
        //-----------------------------------------------------------
        fclose(file_out);
       //-----------------------------------------------------------
        // get which peer to download
       printf("Choose which peer to download:");
       fgets(str,MAXLINE,stdin);
       peernum = atoi(str);
       if(peernum < 0 || peernum > count)
           printf("Invalid input, try again\n");
       strcpy(peerid,peerlist[peernum]);
       printf("you select peer: %s, begin download\n",peerid);

       //begin download
       download(filename, peerid);
       return 1;
    }
    else
    {
        printf("Fail to find file\n");  
                //display in output
        sprintf(msg,"Fail to find file\n");
        fwrite(msg,1,strlen(msg),file_out);
        fclose(file_out);
        return 0;
    }
     return 0;
}

void download(char *filename,char *peerid)
{
    struct sockaddr_un cdaddr;
	int cd_fd;                                    //create a client socket to download from another socket
    char filesize[MAXFILESIZE];
    int size;
    char buf[BUFF_SIZE];
    char msg[MAXLINE] ;
   file_out = fopen("../output.txt","a+");
    if(strcmp(peerid,HOST) == 0)
	{
		printf("Cannot download file from self\n");
		return;
	}
    cd_fd= socket(AF_UNIX,SOCK_STREAM,0);
    if(cd_fd < 0)
        perror("Socket");

    int err;
    //Set socket structure variables
    memset(& cdaddr,0,sizeof( cdaddr));
    cdaddr.sun_family = AF_UNIX;
    strcpy( cdaddr.sun_path, peerid);
    printf("peerid:%s\n",cdaddr.sun_path);

    //Connect to the other client
    err = connect(cd_fd,(const struct sockaddr *)& cdaddr,sizeof( cdaddr));
    if(err < 0)
        perror("Connect");

    //Send  filename
    send(cd_fd,(void *)filename,MAXLINE,0);
    //Receive the filesize
    recv(cd_fd,(void *)filesize,MAXFILESIZE,0);
    printf("Filesize:%s",filesize);
    size = atoi(filesize);
    FILE *file = fopen(filename,"w");
    int recvf = 0;
	int totalb = atoi(filesize);
    while(((recvf = recv(cd_fd,buf,BUFF_SIZE,0)) > 0) && (size > 0))
	{
		fwrite(buf,sizeof(char),recvf,file);
		size -= recvf;
	}
    printf("File received\n");
    //-----------------------------------------------------------
    
    sprintf(msg, "download %s from %s to %s\n" ,filename, peerid, HOST);
    fwrite(msg,1,strlen(msg),file_out);
    fclose(file_out);
    //-----------------------------------------------------------

	fclose(file);

	close(cd_fd);
}