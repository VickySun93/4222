/* server.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

/*-----------------------------------------------------------------------
 *
 * Program: server
 * Purpose: wait for a connection from an echoclient and echo data
 * Usage:   server <appnum>
 *
 *-----------------------------------------------------------------------
 */
void *incoming(int );

struct sockaddr_in serv_addr;

char sendBuff[1025];
char recvBuff[100];
int listenfd = 0, connfd = 0;
char s[INET6_ADDRSTRLEN];


int myport = 0;
char myip[100];

int peerfd[5];
char peeraddr[5][200];
char peername[5][200];
char hostname[128];
struct hostent *he; 
void *interface();
int count=0;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
pthread_t thread2;
int
main(int argc, char *argv[])
{
	 pthread_t thread;
	
	if (argc != 2) {
		(void) fprintf(stderr, "usage: %s <appnum>\n", argv[0]);
		exit(1);
	}
   	//struct sockaddr_in serv_addr;
   	listenfd = socket(AF_INET, SOCK_STREAM, 0);
   	memset(&serv_addr, '0', sizeof(serv_addr));
   	memset(sendBuff,'0', sizeof(sendBuff));
	memset(recvBuff,'0', sizeof(recvBuff));

	
	gethostname(hostname, sizeof hostname);
	//printf("My hostname: %s\n", hostname);   	
	
	
    	// in_addr **addr_list;
	if ((he = gethostbyname(hostname)) == NULL) {  // get the host info
        		perror("gethostbyname");
        		
    	}
   	serv_addr.sin_family = AF_INET;
	//inet_pton(AF_INET, he->h_addr, &serv_addr.sin_addr);
	//memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
	bcopy((char *)he->h_addr, (char *)&serv_addr.sin_addr.s_addr, he->h_length);

      	serv_addr.sin_port = htons(atoi(argv[1]));
    	myport =atoi(argv[1]);
	strcpy (myip,inet_ntoa(serv_addr.sin_addr));

   	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))!=0){perror("bind error");}

   	if(listen(listenfd, 5)!=0){perror("listen");}
   	
	 
//---------------------------------------------------------------------
	/* iterate, echoing all data received until end of file */
	pthread_create(&thread, NULL, interface,NULL);
	
	while(1) {  // main accept() loop
		connfd =0;
		struct hostent *ha;
		socklen_t sin_size;
		struct sockaddr_storage their_addr;
		//printf("1");
		sin_size = sizeof their_addr;
		//fprintf(stdout,"2");
		connfd  = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
		//printf("connfd=%d\n",connfd);
		
		//fprintf(stdout,"3");
		if (connfd  == -1) {
			perror("accept");
			continue;
			
		}else{	
			if(count <6){
			
			
			inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr ),
			s, sizeof s);
			ha = gethostbyaddr(get_in_addr((struct sockaddr *)&their_addr ), sin_size, AF_INET);
			
			printf("got connection from %s %s\n", ha->h_name,s);
			fflush(stdout);

			peerfd[count] = connfd;
			//printf("hostname =%s\n",ha->h_name);
			strcpy(peeraddr[count], s);
			strcpy(peername[count], ha->h_name);
			
			count++; 
			//printf("go to thread2\n");
			pthread_create(&thread2,NULL, incoming, connfd  );
			}else{
				fprintf(stdout,"error:more than 5 connection\n");
				fflush(stdout);
				close(connfd);
			}
		}
		

	
		

		//close(connfd );  // parent doesn't need this
	}
		pthread_exit(NULL);
}

void *incoming(int connfd){
	//if(count >0){
		//int k=0;
		//printf("entered thread2\n");
		//for(k=0; k<count;k++){	
			int len=0;
//change peerfd[k] to connfd
			while((len = recv(connfd , recvBuff, 100, 0)) > 0){
			//printf("received thread2\n");
			if(strncmp(recvBuff, "EXIT",4)==0){
				//fprintf(stdout, "EXIT received\n");
				fflush(stdout);

				int i = 0;
				for(i = 0; i< count ; i++){
					if(peerfd[i] ==  connfd){
						close (connfd);
						fprintf(stdout, "host:%s EXIT\n",peername[i]);
						fflush(stdout);

						int j=0;
						for(j= i ; j< count-1;j++){
							peerfd[j] = peerfd[j+1];
							strcpy(peeraddr[j] , peeraddr[j+1]);
							strcpy(peername[j] , peername[j+1]);
							
						}
						//peeraddr[j] = '\0';
						//peername[j] = '\0';
						count--;
						break;
					}
					//break;
				}	
			}else if(strncmp(recvBuff,"TERMINATE",9)==0){
				//fprintf(stdout, "terminated received\n");
				fflush(stdout);

				int q = 0;
				for(q = 0; q< count ; q++){

				//fprintf(stdout, "peerfd= %d connfd = %d\n",peerfd[q],connfd);
				fflush(stdout);

					if(peerfd[q] ==  connfd){
						//fprintf(stdout, "recevied terminated333\n");
						fflush(stdout);

						close (connfd);
						fprintf(stdout, "host:%s TERMINATE\n",peername[q]);
						fflush(stdout);

						int j=0;
						for(j= q ; j< count-1;j++){
							
							peerfd[j] = peerfd[j+1];
							strcpy(peeraddr[j] , peeraddr[j+1]);
							strcpy(peername[j] , peername[j+1]);
							
						}
						//peeraddr[j] = '\0';
						//peername[j] = '\0';
						count--;
						break;
					}
					
				}

			}
			memset(sendBuff,'0', sizeof(sendBuff));
			memset(recvBuff,'0', sizeof(recvBuff));

			len=0;
			}
		//}
	//}
	pthread_exit(NULL);	
		
}
	void *interface(){
		char buffer[50];
		int len =strlen(buffer);
		while(fgets(buffer, 50, stdin)!=NULL ){
			
			if (len > 0 && buffer[len-1] == '\n')  // if there's a newline
        				buffer[len-1] = '\0';

			char *token;
			if ((token = strtok(buffer, " ")) != NULL) {
    				//printf("token: \"%s\"\n", token);
				if(strncmp(token, "HELP" ,4 ) == 0){
					printf("command:\n1)HELP\n2)EXIT\n3)MYIP\n4)MYPORT\n5)CONNECT host_name port_number\n6)LIST\n7)TERMINATE connection ID\n8)ISALIVE connection ID\n9)CREATOR\n");
					fflush(stdout);

				}else if(strncmp(token, "EXIT" ,4 ) == 0){
					int i =0;
					for(i=0; i <count; i++){
						//fprintf(stdout,"send exit\n");
						fflush(stdout);
						send(peerfd[i],"EXIT",4,0);
						close(peerfd[i]);
					}					

					close(listenfd);
					exit(0);
				}else if(strncmp(token, "MYIP" ,4 ) == 0){
       	 				fprintf(stdout,"%s\n",inet_ntoa(serv_addr.sin_addr));
					fflush(stdout);
	
				}else if(strncmp(token, "MYPORT" , 6) == 0){
					fprintf(stdout,"%d\n",myport);
					fflush(stdout);

				}else if(strncmp(token, "CONNECT" ,7 ) == 0){
					token = strtok(NULL, " ");
					int m = 0;
					int judge = 0;
					if(strcmp(token, hostname)==0){
							fprintf(stdout,"\n Failed \n");
							fflush(stdout);

							judge = 1;

					}
					for(m=0; m<count; m++){
						//printf("check peername\n");
						if(strcmp(token, peername[m]) == 0){
							fprintf(stdout,"\n Failed \n");
							fflush(stdout);

							judge = 1;
						}
						
					}
					if(judge== 0){
					struct sockaddr_in peer_addr;
					struct hostent *hp;
					int sockfd;
					
  					if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
   					{
     					 	fprintf(stdout,"\n Error : Could not create the socket\n");
      						//return 1;
					}
					peerfd[count] = sockfd;
					hp = gethostbyname(token);
					strcpy(peername[count], token);
  					memset(&peer_addr, '0', sizeof(peer_addr));
  					peer_addr.sin_family = AF_INET;
					token = strtok(NULL, " ");
					
  					peer_addr.sin_port = htons(atoi(token));
  					
 			  		bcopy((char *)hp->h_addr, (char *)&peer_addr.sin_addr.s_addr, hp->h_length);
					strcpy(peeraddr[count], inet_ntoa(peer_addr.sin_addr));
					//printf("address =%s\n name=%s\n",peeraddr[count],peername[count]);
					while(1){
  						if (connect(sockfd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == 0)
  						{
							
       							fprintf(stdout,"\n Success \n");
							fflush(stdout);

							count++;
							pthread_create(&thread2,NULL, incoming, sockfd);

       							break;
							
						}
  					}
					}

				
				}else if(strncmp(token, "LIST" ,4 ) == 0){
					int n=0;
					for(n=0; n<count; n++){
						fprintf(stdout,"%d : %s : %s\n",n+1,peername[n],peeraddr[n]);
					
						fflush(stdout);

					}
				}else if(strncmp(token, "TERMINATE" , 9) == 0){
					token = strtok(NULL, " ");
					
					int id = atoi(token);
					if(id > count){	
						//fprintf(stdout,"1\n");
						fprintf(stdout,"there is no valid connection\n");
						fflush(stdout);

					}else{
						send(peerfd[id-1],"TERMINATE",9,0);
						//fprintf(stdout,"SEND TERMINATE\n");
						fflush(stdout);

						close(peerfd[id-1]);
						int j=0;
						for(j= id-1 ; j< count-1;j++){	
							peerfd[j] = peerfd[j+1];
							strcpy(peeraddr[j] , peeraddr[j+1]);
							strcpy(peername[j] , peername[j+1]);		
						}
						count--;
					}
				}else if(strncmp(token, "ISALIVE" ,7 ) == 0){
					token = strtok(NULL, " ");
					int id = atoi(token);
					if(id > count){
						fprintf(stdout,"there is no valid connection\n");
						fflush(stdout);

					}else{
						//send(peerfd[id-1],"ISALIVE",7,0);
						int i = 0;
						int resp ;
						for(i=0; i < 1000; i ++){
							resp = send ( peerfd[id-1], "123", 3 ,MSG_NOSIGNAL );
							if(resp == -1)break;
						}
						if ( resp == -1  ) { 
								fprintf(stdout,"INACTIVE\n");
								fflush(stdout);

								close(peerfd[id-1]);
								int j=0;
								for(j= id-1 ; j< count-1;j++){
									peerfd[j] = peerfd[j+1];
									strcpy(peeraddr[j] , peeraddr[j+1]);
									strcpy(peername[j] , peername[j+1]);
								}
								count--;
						}else{
							fprintf(stdout,"ACTIVE\n");
							fflush(stdout);

	
						}
					}
				}else if(strncmp(token, "CREATOR" ,7 ) == 0){
					fprintf(stdout,"Fangfang Sun, sun375@purdue.edu\n");
					fflush(stdout);

				}
				
			}
			memset(buffer,'0', sizeof(buffer));
			//memset(recvBuff,'0', sizeof(recvBuff));

		}
	 pthread_exit(NULL);

	}
