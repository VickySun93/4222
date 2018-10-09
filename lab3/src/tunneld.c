#include <stdio.h>
#include <stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
 #include <arpa/inet.h>
void *connection(void *remaddr);
#include <sys/time.h>
char serport[128][10] ;
char serip [128][10];
char clientip[128][10];
int clientfd[10];
int serverfd[10];
int count=0;
int listenfd;
int ports[10];
struct sockaddr_in remaddr; 
socklen_t remlen = sizeof(remaddr); 
int main(int argc, char *argv[])
{
	 if (argc < 2) {
    		fprintf(stderr,"ERROR, no port provided\n");
    		exit(1);
  	}

	struct sockaddr_in serv_addr;
	
	if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		return 0; 
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   	serv_addr.sin_port = htons(atoi(argv[1]));
	memset(serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero);
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))< 0) 
	{ 
		return 0; 
	}
//receive request from mytunnel and create thread for every host
 	while(1){
		int recvlen =0;	
		char buf[1024];
		recvlen = recvfrom(listenfd , buf, 1024, 0, (struct sockaddr *)&remaddr, &remlen);	
		struct sockaddr_in *addr = malloc(sizeof(*addr ));
		*addr  = remaddr;
		if(recvlen > 0){
			fprintf(stdout, "%s\n",buf);
			char *con= strtok(buf, " ");
			if(strcmp(con,"Server")==0){
				char *ip = strtok(NULL, " ");
				char*port = strtok(NULL, " ");
				strcpy(serport[count], port);
				strcpy(serip[count] , ip );
			 	pthread_t connectionThread;
				if (pthread_create(&connectionThread, NULL, connection, addr  )) {
    					perror("create Thread error");
    					
  				}
			}
  		}
	}
	return 0;
}
void *connection(void* addr){

 	fd_set readfds; 
	FD_ZERO(&readfds);
	struct timeval tv;
	 tv.tv_sec = 0;
    	tv.tv_usec =500;
	struct sockaddr_in sin;
	int sock;
//set up new port
	if ((sock= socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		perror("socket\n");
	}
	sin.sin_port = htons(0);
	sin.sin_addr.s_addr = 0;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == -1) {
  		if (errno == EADDRINUSE) {
    			perror("Port in use");
  		}
	}
	struct sockaddr_in sin2;
	socklen_t len = sizeof(sin);
	int myid =0;
//retrive new port and send back to mytunnel
	if (getsockname(sock, (struct sockaddr *)&sin2, &len) != -1){
		ports[count] = ntohs(sin2.sin_port);
		clientfd[count] = sock;
		myid = count;
		count++;
  		printf("new port number %d\n", ports[count-1]);
		char sendbuf[1000];
		sprintf(sendbuf,"ACK port : %d",ports[count-1]);
		while(1){
			int a =sendto(listenfd , sendbuf, strlen(sendbuf)+1, 0, (struct sockaddr *)&remaddr, remlen);
			if(a>0)break;
		}
	}
//prepare server information for send and receive
	struct sockaddr_in servaddr;
	int fd= socket(AF_INET, SOCK_DGRAM, 0);
	if (fd  < 0) { 
		perror("fd error\n" ); 
	}			
	servaddr.sin_family = AF_INET;
   	servaddr.sin_port = htons( atoi(serport[myid])   );// This line of code is used to allow OS assign a port number					
	servaddr.sin_addr.s_addr = inet_addr( serip[myid] );
	memset(servaddr.sin_zero, '\0', sizeof servaddr.sin_zero);  

	struct sockaddr_in clientaddr; 
	socklen_t clientlen = sizeof(clientaddr); 
	int recvlen =0;
	int recvlen2=0;
	struct sockaddr_in servaddr2;
	socklen_t servlen2 = sizeof(servaddr2);
	char buf[1024];	
	char buf2[1024];
//handle communication between client and server using select	
	while(1){
	FD_SET(fd, &readfds); 
	FD_SET(sock, &readfds);
	select(fd+1, &readfds, NULL, NULL, &tv);
		recvlen =0;
		recvlen2=0;
		if(FD_ISSET(sock,&readfds)){

			recvlen = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&clientaddr, &clientlen);

			if(recvlen>0){
				//fprintf(stdout, "buf=%s, \n",buf);
				while(1){	
					int len = sendto(fd , buf, recvlen , 0, (struct sockaddr *)&servaddr,sizeof(servaddr)  );
					//printf("mylen=%d\n",len);
					if (len > 0) {
						break;
					}
				}
  			}
		}
		if(FD_ISSET(fd,&readfds)){

			recvlen2 = recvfrom(fd , buf2, 1024, 0, (struct sockaddr *)&servaddr2, &servlen2 );

			if(recvlen2 >0){	
				//fprintf(stdout, "len2=%d\n",recvlen2 );
				while(1){	
				int len = sendto(sock, buf2, recvlen2 , 0, (struct sockaddr *)&clientaddr, clientlen);
					if (len > 0) {
						break;
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}
