#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


FILE* fptr = NULL;
pthread_t track;

int filesize = 0;
char *map;
int client_socket;

struct sockaddr_in server_address;
struct stat statbuf;
int total_num_pkts=0;
struct timespec start;

typedef struct pkt {
    uint32_t seqNum;   
    int flag;               
    char buf[1464]; 
}pkt;

typedef struct firstpkt {
    uint32_t seqNum;   
    int filesize;               
    char buf[1464]; 
}firstpkt;

int main(int argc, char *argv[])
{
struct stat st;
struct hostent *server;
struct firstpkt firstpkt;
int port,i,fp;
long int window_size=64*1024*1024;
//command line input
if(argc < 4)
{
	printf("\nError in command");
	exit(0);
}

//get the host name
if((server=gethostbyname(argv[1]))==NULL){
	perror("\nUnknown host!");
	exit(0);
}

//get the port number
port=atoi(argv[2]);

//socket creation and binding
if ((client_socket = socket(AF_INET,SOCK_DGRAM, 0)) < 0)
{
	perror("Error while creating socket");
	exit(1);
}
printf("\n>>>Socket created");

//clear memory for server address
memset(server_address.sin_zero,'\0',sizeof server_address.sin_zero);

//Server address setup: Reference:Beej Section 5.3
server_address.sin_family = AF_INET;
server_address.sin_port = htons(port);
bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr,server->h_length);

//setting window size
setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, &window_size, sizeof window_size);
setsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, &window_size, sizeof window_size);
printf("\n>>>Buffer size increased");
    
if ((fptr = fopen(argv[3], "r")) == NULL)
{
        perror("error in fopen");
}
	
if(stat(argv[3],&st)!=0)
{
	perror("\nFile status not available");
	exit(0);
}

filesize=st.st_size;
printf("\n>>>Filesize : %d.", filesize);

//opening file for changing status
if((fp = open(argv[3], O_RDONLY))<0)
{
        perror("Error: while opening the file\n");
}   
if(fstat(fp,&statbuf) < 0)
{
	perror("ERROR: while file status\n");
}   
if((map = mmap((caddr_t)0, statbuf.st_size , PROT_READ , MAP_SHARED , fp , 0 ))== MAP_FAILED )
{
	perror("ERROR : mmap error \n");
}  

//sending filesize 
printf("\n>>>Sending Filesize"); 
memcpy( firstpkt.buf,&map[0] , 1464 );
firstpkt.seqNum = 0;
firstpkt.filesize=filesize;
for(i=0 ; i<10 ; i++)
{
        if (sendto(client_socket,&firstpkt,sizeof(firstpkt), 0,(struct sockaddr *) &server_address,sizeof(server_address)) < 0)
            perror("error sending filesize");
}
    
  
if(clock_gettime(CLOCK_REALTIME , &start) == -1){
	perror("clock get time");
	exit(0);
}
printf("\n>>>Start time : 	%f",start.tv_sec+(double)(start.tv_nsec)/1e9);

printf("\n>>>Sending file"); 

printf("\n>>>Checking for nacks");
int left_byte , frag = 1464,n=0;
pkt nack_pkt ;
int nack;
int fl;
socklen_t p= sizeof(server_address);
total_num_pkts =  filesize / 1464;
left_byte = filesize % 1464;
while (1) {
	fl=1;
	n = recvfrom(client_socket,&nack,sizeof(int),0,(struct sockaddr *)&server_address,&p);
	printf("\n nack %d",nack);
        if(n<0)
	{
            perror("rcv from\n");
        }
        if( nack < 0)
	{
            printf("\n>>>File Sent!!\n\n"); 
            pthread_exit(0);
        }
        if(total_num_pkts==nack && left_byte!=0)
        {
        		frag = left_byte;
			fl=0;
        }
        memcpy( nack_pkt.buf,&map[nack*1464] , frag );
        nack_pkt.seqNum = nack;
	nack_pkt.flag=fl;
        printf("\nsending packet %d",nack_pkt.seqNum);
        if((n = sendto(client_socket,&nack_pkt,sizeof(pkt), 0,(struct sockaddr *) &server_address,p))< 0)
	{
            	perror("sendto");      
		exit(0);
	}
}  

    munmap(map, statbuf.st_size);
    close(client_socket);
    fclose(fptr);
    return 0;
}
