#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct packet {
    uint32_t seqNum;   
    int flag;                                  
    char buf[1464];
}packet;

typedef struct firstpkt {
    uint32_t seqNum;   
    int filesize;               
    char buf[1464]; 
}firstpkt;

FILE *file;
char *track_pkt;
int miss=1;
struct stat statbuf;

struct timespec fin;

pthread_t nack;
int filesize;
int sockfd;
struct sockaddr_in server_address;

int totalPacketsArrived =0 , totalpackets=0;

void* sending_nack(){
    int n=0;
    int na;
    printf("\n>>>Entered NACK Thread");
    while(1)
    {
        if(totalPacketsArrived == totalpackets+1 ) {
            printf("\n>>>File Received!!\n\n");
            pthread_exit(0);
        }
        usleep(100);
        pthread_mutex_lock(&mutex);
	int i;
	for ( i = miss; i <= totalpackets ; i++)
	{
		if (track_pkt[i] == 0 )
		{
			if (i==totalpackets)
				miss=1;
			else
				miss=(i+1);
                na=i;
		break;
		}
	}
	if(i > totalpackets){
	miss=1;
	na=-1;}
        pthread_mutex_unlock(&mutex);

        if(na >= 0 && na<=totalpackets){
            if((n = sendto(sockfd, &na ,sizeof(int),0,(struct sockaddr *)&server_address,sizeof(server_address)))<0)
                perror("\nError sending NACK");
        }
        
    }
}


int main(int argc, char *argv[])
{
    int portno,i=0;
    int n;
    packet pkt; 
    socklen_t p;
    firstpkt firstpkt;
    p = sizeof(struct sockaddr_in);
    
    if (argc < 2) {
        fprintf(stderr,"Missing Port Number\n");
        exit(1);
    }
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
        perror("ERROR opening socket");
    memset(&server_address,'\0', sizeof(server_address));
    portno = atoi(argv[1]);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(portno);
    
    long int ws = 64*1024*1024;
    
    setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(char *)&ws,(long int)sizeof(ws));
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(char *)&ws,(long int)sizeof(ws));
       
    if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
        perror("ERROR on binding");

    file = fopen("received_file" , "w+");

//First packet containing file size    
    n = recvfrom(sockfd,&firstpkt,sizeof(firstpkt),0,(struct sockaddr *)&server_address,&p);

        if(firstpkt.seqNum==0){
			filesize=firstpkt.filesize;
            		fseek( file , 1464*firstpkt.seqNum , SEEK_SET  );
            		fwrite(&firstpkt.buf , 1464 , 1 , file);
            		fflush(file);
            		totalPacketsArrived++;
}

    printf("\n>>>Receiving file of size: %d",filesize);
    totalpackets = ceil(filesize / 1464);
    printf("\n>>>Total number of Packets: %d",totalpackets);
    
    track_pkt = calloc(totalpackets , sizeof(char));
    pthread_create(&nack, 0, sending_nack , (void*)0 );
    
    track_pkt[0]=1;
    int left_bytes=filesize%1464;
    
    while (1)
    {

        if((n = recvfrom(sockfd,&pkt,sizeof(packet),0,(struct sockaddr *)&server_address,&p))<0)
	perror("\nError Receiving Packets");
             
        pthread_mutex_lock(&mutex); 
        if(pkt.seqNum>=0 && pkt.seqNum<=totalpackets){
		if (track_pkt[pkt.seqNum] == 0){
			track_pkt[pkt.seqNum] = 1;
            		fseek( file , 1464*pkt.seqNum , SEEK_SET  );
			if(pkt.flag==1)
            		fwrite(&pkt.buf , 1464 , 1 , file);
			if(pkt.flag==0)
			fwrite(&pkt.buf , left_bytes , 1 , file);
            		fflush(file);
            		totalPacketsArrived++;
        } 
}
        pthread_mutex_unlock(&mutex);


int signal=-1;
    if(totalPacketsArrived == totalpackets+1 ){
	printf("\n>>>Sending nack of -1 to signal that we received the whole file");
        for(i=0 ; i<5 ; i++){
            if((n = sendto(sockfd,&signal,sizeof(int), 0,(struct sockaddr *) &server_address,sizeof(server_address)))<0)
                perror("\nError sending final signal");
        }
        
        break;
    }
}
if(clock_gettime(CLOCK_REALTIME,&fin)<0){
	perror("\nGetting clock time");
	exit(0);
}
printf("\n>>>End time : 	%f",fin.tv_sec+(double)(fin.tv_nsec)/1e9);
pthread_join(nack, 0);
fclose(file);
close(sockfd);
return 0;
}
