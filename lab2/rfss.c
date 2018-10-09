/**********************************
 *
 * CS 422 Data Communications and Computer Networks
 * Lab1
 * rfss_thread.c
 *
 **/
#include <dirent.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/time.h>
#define NUM_CONNECTIONS 5
  #include <sys/sendfile.h>
/*
Structure to keep track of the connections
*/
typedef struct connections {
  char *host;
  char *ip;
  int socketfd;
  struct connections *next;
} connections_t;


char myfile[20][50];
int filesize[20];
int countfile = 0;
int active_connections = 0;
connections_t *g_table;
pthread_mutex_t lock;

/* Flag to check in the other threads whether the user has
 typed the EXIT command*/
int exitThread = 0;

/* Forward function declarations*/
void ctable_initialize(connections_t *ctable);
int ctable_add(connections_t **ctable, char *host_name, char *ip,
    int socket_fd);
int ctable_delete(connections_t **ctable, int connection_id);
int ctable_list(connections_t *ctable);
bool isDup(connections_t *ctable, char *host_name);
void *connection(void *sock);
int createConnectionThread(int sockfd);
int connectMachine(char *hostName, int portNo);
int terminateMachine(int connectionId);
void exitMachine();
int establishConnection(struct sockaddr_in addr, int sockfd);
int terminateConnection(int sockfd);
void *userInterface ( );
void generatefile(char*a, int b);
int uploadfile(int connectionId,char* filename  );
int printfile(int id);
void sendfilename(int sockfd);
int sendrequst(int connectionId ,char * name);
int getupload(int sockfd, int size,char* name);
/* Function: ctable_initialize
 * ---------------------------
 * ctable: The connection table to be initialized
 */
void ctable_initialize(connections_t *ctable) {
  ctable = NULL;
}

/*
 * Function: ctable_add
 * --------------------
 * adds an entry(hostname, ip, socket_fd to the connections table
 * ctable: connections table
 * host_name: hostname of the connection to be added
 * ip: ip address of the connection to be added
 * socket_fd: socket file descriptor of the connection
 * returns: 1 if the connection addition was successful otherwise -1
 */
int ctable_add(connections_t **ctable, char *host_name, char *ip, 
    int socket_fd) {
  pthread_mutex_lock(&lock);     
  fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK);
  active_connections++;
  struct connections *curr, *temp;
  temp = malloc(sizeof(struct connections));
  if (temp == NULL) {
    return -1;
  }

  temp->socketfd = socket_fd;
  temp->host =  (char *) malloc(100);
  temp->ip =  (char *) malloc(12);
  strcpy(temp->host, host_name);
  strcpy(temp->ip, ip);
  temp->next = NULL;
    
  if (*ctable == NULL) {
    *ctable = temp;
    pthread_mutex_unlock(&lock);
    return 1;
  }
  
  curr = *ctable;
  while (curr->next != NULL) {
    curr = curr->next;
  }
  curr->next = temp;
  pthread_mutex_unlock(&lock);     
  return 1;
}

/*
 * Function: ctable_delete
 * --------------------
 * deletes an entry(hostname, ip, socket_fd to the connections table
 * ctable: connections table
 * connections_id: Index of the connection in the link list
 * returns: 1 if the connection deletion was successful otherwise -1
 */
int ctable_delete(connections_t **ctable, int connection_id) {
  if (*ctable == NULL) {
    return -1;
  }
  int i = 1;
  pthread_mutex_lock(&lock);
  struct connections *curr = *ctable, *prev;
  if (connection_id == 1) {
    *ctable = curr->next;
    active_connections--;
    free(curr);
    pthread_mutex_unlock(&lock);
    return 1;
  }
  for (i = 1; i < connection_id; i++) {
    prev = curr;
    curr = curr->next;
  }
  prev->next = curr->next;
  free(curr);
  active_connections--;
  pthread_mutex_unlock(&lock);
  return 1;
}

/*
 * Function: ctable_list
 * --------------------
 * lists the entries in the connections table
 * ctable: connections table
 * returns: 1 if the connection table exists otherwise -1
 */
int ctable_list(connections_t *ctable) {
   if (ctable == NULL) {
     printf("No active connections\n");
     return -1;
   }
   int count = 1;
   while (ctable != NULL) {
     printf("%d", count);
     printf(":%s", ctable->host);
     printf(":%s\n", ctable->ip);
     count++;
     ctable = ctable->next;
   }
   return 1;
}

/*
 * Function: isDup
 * --------------------
 * checks if a hostname is already connected
 * ctable: connections table
 * host_name: hostname
 * returns: True if there is a duplicate entry else False
 */
bool isDup(connections_t *ctable, char *host_name) {
  while (ctable != NULL) {  
    if (strcmp(ctable->host, host_name) == 0) {
      return true;
    }
    ctable = ctable->next;
  }
  return false;
}

/*
 * Function: error
 * --------------------
 * prints the error message and exits
 * msg: message
 */
void error(const char *msg) {
  perror(msg);
  exit(1);
}
 
/*
 * Function: connectMachine
 * --------------------
 * This function is called when the user enters the command CONNECT
 * hostname: the hostname that the machine wants to connect to
 * portNo: the prt number of the destination
 * returns 1 on successful connection establishment otherwise -1
 */
int connectMachine(char *hostName, int portNo) {
  if (active_connections >= NUM_CONNECTIONS) {
    fprintf(stderr, "You cannot exceed maximum no of. connections");
    return -1;
  }
  
  int sockfd;
  struct sockaddr_in dest;
  struct in_addr **addr_list;
  char recv_buf[100];
  struct hostent *dest_he = NULL;
  char ownHostName[128];
  long int in_addr;

  dest_he = gethostbyname(hostName);
  if (dest_he == NULL) {
     fprintf(stderr,"ERROR, no such host as %s\n", hostName);
     return -1;
  }
   
  /* Check if a connection already exists*/
  if (isDup(g_table, hostName)) {
    printf("\nDuplicate Connection!\n");
    return -1;
  }

  /* Get your own host name*/
  gethostname(ownHostName, sizeof (ownHostName));
  if (strcmp(ownHostName, hostName) == 0) {
    printf("\nCannot connect to your ownself!\n");
    return -1;
  }

  /*Create a socket*/
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("Socket");
    return -1;
  }

  memset(&dest, 0, sizeof(dest));
  bzero(&dest, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = htons(portNo);

  /* Get IP address from hostname*/
  addr_list = (struct in_addr **)dest_he->h_addr_list;
  memcpy(&in_addr, addr_list[0], sizeof(in_addr));
  dest.sin_addr.s_addr = in_addr;
  char *ip = inet_ntoa(*addr_list[0]);
  
  if (connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 ) {
    perror("Connect ");
    return -1;
  }

  if (send(sockfd, "CONNECT", strlen("CONNECT"), 0) < 0) {
    return -1;
  }

  /* Wait till a success message is received*/
  if (recv(sockfd, recv_buf, sizeof(recv_buf), 0) < 0) {
      return -1;
  }
 
  if (strcmp(recv_buf, "SUCCESSFUL") == 0) {
    if (createConnectionThread(sockfd) < 1) {
      return -1;
    }
    ctable_add(&g_table, hostName, ip, sockfd);
    return 1;
  }
  return -1;
}

/*
 * Function: terminateMachine
 * --------------------
 * This function is called when the user enters the command TERMINATE
 * connectionID: the connection ID of the machine that it wants to terminate a
 * connection with.
 * returns 1 on successful connection termination otherwise -1
 */
int terminateMachine(int connectionId) {
  if (connectionId > active_connections) {
    fprintf(stderr, "Connection does not exist");
    return -1;
  }

  int i;
  struct connections *curr = g_table;
  for (i = 1; i < connectionId; i++) {
    curr = curr->next;
  }
  if (send(curr->socketfd, "TERMINATE", strlen("TERMINATE"), 0) < 0) {
    fprintf(stderr, "Send Error");
  }

  close(curr->socketfd);
  ctable_delete(&g_table, connectionId);
  return 1;
}

int printfile(int connectionId){
	if (connectionId > active_connections) {
   		 fprintf(stderr, "Connection does not exist");
    		return -1;
  	}

  	int i;
  	struct connections *curr = g_table;
  	for (i = 1; i < connectionId; i++) {
    	curr = curr->next;
  	}
	//fprintf(stdout,"send request\n");
  	if (send(curr->socketfd, "GETFILENAME", strlen("GETFILENAME"), 0) < 0) {
    		fprintf(stderr, "Send Error");
  	}


  	return 1 ;

}
int sendrequst(int connectionId ,char * name){
	if (connectionId > active_connections) {
    		fprintf(stderr, "Connection does not exist\n");
    		return -1;
  	}
	int k;
  	struct connections *curr = g_table;
  	for (k = 1; k < connectionId; k++) {
    		curr = curr->next;
  	}
	char a [1024];
	sprintf(a, "REQUEST %s", name);
	send(curr->socketfd, a,strlen(a) , 0);
	return 1;
}


int uploadfile(int connectionId,char* filename  ){
	struct timeval tvalBefore, tvalAfter;

	if (connectionId > active_connections) {
    		fprintf(stderr, "Connection does not exist\n");
    		return -1;
  	}
	int datasize=0;
	int i=0;
	int check=0;
	for(i=0; i < countfile; i++){
		if(strcmp(myfile[i],filename)==0){
			datasize = filesize[i];
	
			check= 1;
			break;
		}
	}
	if(check ==0){
		fprintf(stderr, "File was inaccessible\n");
		return -1;
	}
	int fildes =open( "config.txt" , O_RDONLY );
	char getsize[100];
	read(fildes,getsize, 100) ;
//buffer size
	int size =  atoi(getsize);		
//sendbuff
	char sendbuff[1400];

//openuploadfile from upload
	char filepath[1024];
	sprintf(filepath, "Upload/%s",filename);
	int uploadfile = open( filepath , O_RDONLY );
	int k;
  	struct connections *curr = g_table;
  	for (k = 1; k < connectionId; k++) {
    		curr = curr->next;
  	}
//send upload to u
	//strcpy(sendbuff, "UPLOADTOU");
	sprintf(sendbuff, "UPLOADTOU %d %s",datasize, filename);

	//fprintf(stdout,"datasize= %d\n",datasize);
	//fprintf(stdout,"len=%d\n",(int)strlen(sendbuff));

	send(curr->socketfd, sendbuff, strlen(sendbuff), 0);
//send filesize
	//char a[1400];
	//sprintf(a, "%d %s",datasize, filename);
	//int len =  strlen(a);
	//a[len] = '\0';
	//fprintf(stdout,"len=%d\n",len);
	//send(curr->socketfd, a, len, 0);
//send datachunks

char buff[1400];
int remaindata = 0;
	 gettimeofday (&tvalBefore, NULL);
	while (1) {
		
 		memset(buff, 0, sizeof(buff));
		


    		int bytes_read = read(uploadfile , buff, size );
		//fprintf(stdout, "lll=%d\n",bytes_read );
		//buff[bytes_read] = '\0';
		//remaindata = remaindata +bytes_read;
    		if (bytes_read == 0) 
        			break;

    		if (bytes_read < 0) {
        			perror("error reading file");
			//break;
    		}
	
    		void *p = buff;
    		while (bytes_read > 0) {
				//int bytes_written = send(curr->socketfd, p, bytes_read, 0);
        			int bytes_written = write(curr->socketfd, p, bytes_read);
        			if (bytes_written <= 0) {
           				//perror("error writing file");

				}else{
					remaindata = remaindata +bytes_written ;
					bytes_read =bytes_read - bytes_written;
				}
				
        				p += bytes_written;
        			
				
    		}
	}
	//fprintf(stdout,"remain=%d\n",remaindata );
		

	
gettimeofday (&tvalAfter, NULL);
double tv_usec= ((tvalAfter.tv_sec - tvalBefore.tv_sec)*1000000
           +tvalAfter.tv_usec) - tvalBefore.tv_usec;
//double tv_sec = ((tvalAfter.tv_usec - tvalBefore.tv_usec)/1000000
 //          +tvalAfter.tv_sec) - tvalBefore.tv_sec;
double rate =  datasize*8 / (tv_usec);
char hostname[128];
gethostname(hostname, sizeof hostname);
	
	fprintf(stdout, "Tx (%s): %s -> %s, File Size: %d Bytes, Time Taken: %lf useconds, Tx Rate: %lf bits/microsecond.\n", 
	hostname, hostname,curr->host, datasize, tv_usec, rate);
	fflush(stdout);
	close(uploadfile );
	return 1;
}
/*
 * Function: exitMachine
 * --------------------
 * This function is called when the user enters the command EXIT
*/
void exitMachine() {
  if (active_connections == 0) {
     exitThread = 1;
    return;
  }
  int total_connections = active_connections;
  int i;
  for (i = 1; i <= total_connections; i++) {    
    terminateMachine(1);
  }
  return;
}

/*
 * Function: establishConnection
 * --------------------
 * This function is called when a peer requests a connection to be established
 * addr: peer's ip address
 * sockfd: socket file descriptor of the accepted connection
 * retuurns  1 on successful connection establishment otherwise -1
 */
int establishConnection(struct sockaddr_in addr, int sockfd) {
  if (active_connections >= NUM_CONNECTIONS) {
    return -1;
  }

  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
  struct in_addr cl_addr;
  inet_aton(ip, &cl_addr);
  struct hostent *he;
  if ((he = gethostbyaddr(&cl_addr, sizeof(cl_addr), AF_INET)) != NULL) {
    ctable_add(&g_table, he->h_name, ip, sockfd);
    printf("Established connection with %s\n",he->h_name);
    return 1;
  }
  return -1;
}

/*
 * Function: terminateConnection
 * --------------------
 * This function is called when a peer requests a connection to be terminated
 * sockfd: socket file descriptor of the connection to be terminated
 * retuurns  1 on successful connection termination otherwise -1
 */
int terminateConnection(int sockfd) {
  int i;
  struct connections *curr = g_table;
  for (i = 1; i < active_connections; i++) {
    if (curr->socketfd == sockfd) {
      break;
    }
    curr = curr->next;
  }
  if (curr == NULL) {
    return -1;
  }
  
  close(curr->socketfd);
  char *hostName = curr->host;
  ctable_delete(&g_table, i);
  printf("Terminated connection with %s\n",hostName);
  return 1;
}
void sendfilename(sockfd){

 // 	DIR *dir;
//	struct dirent *ent;
	//char cwd[1024];
	//getcwd(cwd, sizeof(cwd));	
	//sprintf(cwd, "%s/U", cwd);

	/*if ((dir = opendir ("./Upload")) != NULL) {
		send(sockfd, "FILECOMING", 10, 0);

    		while ((ent = readdir (dir)) != NULL) {
    			fprintf (stdout,"%s\n", ent->d_name);
			
			int bytes_written = send(sockfd, ent->d_name, strlen(ent->d_name), 0);
                		if (bytes_written <= 0) {
           			perror("error sending file");

			}

  		}
  		closedir (dir);
	}else {
  	  	perror ("could not open\n");
  	
	}  	 */
	int i=0;
	char a[1024];
	sprintf(a, "FILECOMING %d",countfile);
	
	
	for(i=0; i< countfile; i++){
	
		//sprintf(b, "%s\n",myfile[i]);
		sprintf(a, "%s %s",a,myfile[i]);

		//int bytes_written = send(sockfd,b  ,strlen( b) , 0);
		//fprintf(stdout,"bytesend=%d\n",bytes_written);
                		//if (bytes_written <= 0) {
           		//	perror("error sending file");

			//}

		
	}
	
	send(sockfd, a, strlen(a), 0);
}



int getupload(int sockfd, int size, char* filename){
	//fprintf(stdout,"111\n");
	//fflush(stdout);
	//fflush(stdout);

	struct timeval tvalBefore, tvalAfter; 
	int i;
  	struct connections *curr = g_table;
  	for (i = 1; i < active_connections; i++) {
    		if (curr->socketfd == sockfd) {
      			break;
    		}	
    		curr = curr->next;
  	}
  	if (curr == NULL) {
    		return -1;
  	}
	int fildes =open( "config.txt" , O_RDONLY );
	char getsize[100];
	read(fildes,getsize, 100) ;
//buffer size
	int buffersize =  atoi(getsize);	

	/*char recvbuff[128];
	 memset(recvbuff, 0, sizeof(recvbuff));
//fprintf(stdout,"222\n");

	int r=0;
	while(1){
		if((r = recv(sockfd, recvbuff, sizeof(recvbuff), 0))>0){
			fprintf(stdout,"r=%d\n",r);
			fprintf(stdout,"recvbuff=%s\n",recvbuff);

			break;
		}
	}
	*/
//fprintf(stdout,"333\n");

	//fflush(stdout);
	// recv(sockfd, recvbuff, sizeof(recvbuff), 0);
	//char*  a= strtok(recvbuff," ");
	//printf("buff = %s\n",recvbuff);
	//fflush(stdout);

	//char *size = sizesize;
	//char *filename =name;
	char filepath[150];
	sprintf(filepath, "Download/%s",filename);
 	FILE * newfile= fopen(filepath, "w");
	if (newfile == NULL)
	{
    		fprintf(stderr,"Error opening file!\n");
    		exit(1);
	}
	int remain_data = size;
	//int filesize =remain_data;
	int len=0;

	fflush(stdout);
 	//fprintf(stdout,"remain=%d\n", remain_data);
char buff[1400];
//fprintf(stdout,"444\n");
//int countdata=0;
	gettimeofday (&tvalBefore, NULL);
 	while ( remain_data > 0)
		
        {	
		memset(buff, 0, sizeof(buff));
		
		if( (len = recv(sockfd, buff, buffersize , 0)) > 0  ){
			remain_data  =  remain_data -len;
			fwrite(buff, sizeof(char),len, newfile);
			//countdata = len +countdata;

			/*(while(1){
				if(fwrite(buff, sizeof(char),len, newfile) >0){
					break;
               			}
			}*/

             		//fprintf(stdout, "Receive %d bytes and we hope :%d bytes\n", len, remain_data);
			//fflush(stdout);
		}
	       
        }
	gettimeofday (&tvalAfter, NULL);
//fprintf(stdout,"coutndata=%d\n",countdata );





char hostname[128];
gethostname(hostname, sizeof hostname);
double  tv_usec= ((tvalAfter.tv_sec - tvalBefore.tv_sec)*1000000
           +tvalAfter.tv_usec) - tvalBefore.tv_usec;
//double tv_sec = ((tvalAfter.tv_usec - tvalBefore.tv_usec)/1000000
       //    +tvalAfter.tv_sec) - tvalBefore.tv_sec;
double rate = size*8 / (tv_usec);


	fprintf(stdout,"Rx (%s): %s -> %s, File Size: %d Bytes, Time Taken: %lf useconds, Rx Rate: %lf bits/microsecond.\n", 
	hostname, curr->host,hostname, size , tv_usec, rate);
	fflush(stdout);
	fclose(newfile);
	return 1;
}


/* Function: createConnectionThread
 * --------------------
 * This function is called to create the connection thread
 * sockfd: socket file descriptior
 * returns 1 if the thread is successfully created otherwise -1
 */
int createConnectionThread(int sockfd) {
  pthread_t connectionThread;
  int *arg = malloc(sizeof(*arg));
  *arg = sockfd;

  if (pthread_create(&connectionThread, NULL, connection, arg)) {
    perror("Thread");
    return -1;
  }
  return 1;
}

/*
 * Function: connection
 * --------------------
 * Function that is called when the thread is created after a connection has 
 * been established. This function is passed as an argument to pthread_create()
 * sock: socket file descriptior
 */
void *connection(void *sock) {
  	int sockfd = *((int *) sock);
  	free(sock);
  	char recv_buf[1400];
	
  	while (exitThread == 0) {
    		int status  = fcntl(sockfd, F_GETFL);
    		if (status == -1) {
      			break;
    		}
    		memset(recv_buf, 0, sizeof(recv_buf));
		int length=0;
    		if ((length=recv(sockfd, recv_buf, sizeof(recv_buf), 0)) > 0) {
			//fprintf(stdout,"len111=%d\n",length);
			//fprintf(stdout,"recv_buf111=%s\n",recv_buf);
		
			char *buff= strtok(recv_buf, " ");
      			if (strcmp(buff, "TERMINATE") == 0) {
        			terminateConnection(sockfd);
        			break;
      			}else if (strcmp(buff, "UPLOADTOU") == 0) {
				//fprintf(stdout,"length=%d\n",length);
				char *size = strtok(NULL," ");
				char *name = strtok(NULL," ");


				fprintf(stdout,"upload to me begin\n");
				fflush(stdout);
        			if(getupload(sockfd, atoi(size), name) ==1){
			   		fprintf(stdout, "get file %s successfull \n",name);}
				else{
			    		fprintf(stdout, "get file %s failed\n",name);
				}
     				//fprintf(stdout,"6666\n");
      			}else if (strcmp(buff, "GETFILENAME") == 0) {
				//fprintf(stdout,"777\n");

				//fprintf(stdout,"GETFILENAME\n");
        			sendfilename(sockfd);
			}else if (strcmp(buff, "FILECOMING") == 0) {
				int number = atoi(strtok(NULL," "));
				//char recvbuff[100];
				//int len=0;
				while ( number  > 0)
       	 			{	
					char* a =strtok(NULL," ");
					fprintf(stdout, "%s\n", a);
               				number--;
        			}
      			}else if (strcmp(buff, "REQUEST") == 0) {
				
				char *filename = strtok(NULL," ");
				fprintf(stdout,"file %s requested\n",filename );
				int i;
  				struct connections *curr = g_table;
  				for (i = 1; i < active_connections; i++) {
    					if (curr->socketfd == sockfd) {
      						break;
    					}
    					curr = curr->next;
  				}
  				if (curr == NULL) {
    					perror("connection not exsit\n");
  				}
				
				if (uploadfile(i,filename ) > 0) {
            					printf("Send file Successful\n");
	  			} else {
	    				printf("Send file Unsuccessful\n");
	  			}

				

      			}else{
				
			}
			//fprintf(stdout,"888\n");

    		}//fprintf(stdout,"999\n");

  	}
  	pthread_exit(NULL);
}
void generatefile(char*filename, int size){

	int i=0;
	int check=0;
	for(i=0; i<countfile; i++){
		if(strcmp(filename, myfile[i]) == 0){
			check =1;
			char cmdLine[1024];
        			sprintf(cmdLine, "dd if=/dev/zero of=Upload/%s count=%d",
                           	filename, size*2);
         		system(cmdLine);
			
			filesize[i]=size*1024;
			


		}
	
	}
         if(check == 0){
		char cmdLine[1024];
		sprintf(cmdLine, "dd if=/dev/zero of=Upload/%s count=%d",
                           filename, size*2);
         	system(cmdLine);
		strcpy(myfile[countfile], filename);
		//printf("addfile = %s\n",myfile[countfile]);
		filesize[countfile]=size*1024;
		//fprintf(stdout,"size = %d\n",filesize[countfile]);
		countfile++;

	}
}




/*
 * Function: userInterface
 * --------------------
 * This function is called when the usr interface thread is created
 */
void *userInterface ( ) {
 printf("\n Following commnads are supported:"
       "\n1. LIST"
       "\n2. CONNECT hostname portno"
       "\n3. TERMINATE connection_ID"
       "\n4. EXIT"
	"\n5. GENERATE file_name file_size"
	"\n6. UPLOAD connection_ID upload_file_name"
	"\n7. FILES connection_ID"
	"\n8, DOWNLOAD connection_ID file_name\n");

  while (1) {
    char buf[100];
    fgets(buf, sizeof(buf), stdin);
    if (buf[0] != '\n') {
      buf[strcspn(buf, "\n")] = 0;
      char *command  = strtok(buf," ");
//
      if (strcmp(command, "CONNECT") == 0) {
        char *hostName = strtok(NULL," ");
        if (hostName != NULL) {
          char *portNo = strtok(NULL," ");
	  if (portNo != NULL) {
	    if (connectMachine(hostName, atoi(portNo)) > 0) {
	      printf("Connection Successful\n");
	    } else {
	      printf("Connection Unsuccessful\n");
	    }
          }
        }
//
      } else if (strcmp(command, "LIST") == 0) {
	ctable_list(g_table);
//
      } else if (strcmp(command, "TERMINATE") == 0) { 
        char *connectionId = strtok(NULL," ");
	if (connectionId != NULL) {
	  if (terminateMachine(atoi(connectionId)) > 0) {
            printf("Termination Successful\n");
	  } else {
	    printf("Termination Unsuccessful\n");
	  }
	}
//
      } else if (strcmp(command, "EXIT") == 0) {
        exitMachine();
	exitThread = 1;
        pthread_exit(NULL);
	}else if (strcmp(command, "GENERATE")==0){
		char *filename= strtok(NULL," ");
		char *filesize = strtok(NULL," ");
		int size = atoi(filesize);
		generatefile(filename, size);

	}else if(strcmp(command, "UPLOAD")==0){
		char *connectionId= strtok(NULL," ");
		char* uploadfilename = strtok(NULL," ");
		fprintf(stdout,"start uploading file %s\n",uploadfilename);
		if (connectionId != NULL) {
	  		if (uploadfile(atoi(connectionId),uploadfilename  ) > 0) {
            			printf("Send file Successful\n");
	  		} else {
	    			printf("Send file Unsuccessful\n");
	  		}
		}
	}else if(strcmp(command, "FILES")==0){
			//fprintf(stdout,"111\n");
		char *connectionId= strtok(NULL," ");
		//fprintf(stdout,"111\n");
		if (connectionId != NULL) {
	  		printfile(atoi(connectionId));
		}else{
			perror("connectionID is NULL\n");
		}
	}else if(strcmp(command, "DOWNLOAD")==0){
			//fprintf(stdout,"111\n");
		char *connectionId= strtok(NULL," ");
		char *name = strtok(NULL, " ");
		fprintf(stdout,"start downloading file %s\n",name );
		if (connectionId != NULL) {
			if(name !=NULL){
	  			sendrequst(atoi(connectionId),name);
			}else{
				perror("file_name is NULL\n");

			}
		}else{
			perror("connectionID is NULL\n");
		}
	}

    }
    fflush(stdin);
  }
}

int main(int argc, char *argv[]) {
  pthread_t uiThread;
  int serverfd, portno;
  struct sockaddr_in serv_addr, cli_addr;
 
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
  
  serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverfd < 0) {
     error("ERROR opening socket");
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(serverfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }
  
  ctable_initialize(g_table);

  if (pthread_mutex_init(&lock, NULL) != 0) {
    fprintf(stderr,"mutex init failed\n");
    return 1;
  }

  if (pthread_create(&uiThread, NULL, userInterface, NULL)) {
    fprintf(stderr, "Error creating user interface\n");
    return 1;
  }
  
  if (listen (serverfd, NUM_CONNECTIONS) < 0) {
    error ("listen");
  }

  /* Set the server socket to be non blocking so that we can exit loop*/
  if (fcntl(serverfd, F_SETFL, fcntl(serverfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
    error("nonblocking accept");
  }

  while (exitThread == 0) {
    int new;
    size_t size = sizeof(cli_addr);
    if ((new = accept (serverfd, (struct sockaddr *) &cli_addr,
         (socklen_t*) &size)) > 0) {
       if (establishConnection(cli_addr, new) == 1) {
         if (createConnectionThread(new) < 1) {
           send(new, "NOT SUCCESSFUL", strlen("NOT SUCCESSFUL"), 0);
         } else { 
           send(new, "SUCCESSFUL", strlen("SUCCESSFUL"), 0);
         }
       } else {
         send(new, "NOT SUCCESSFUL", strlen("NOT SUCCESSFUL"), 0);
       }
    } else if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
      error("accept");
    }
  }
  close(serverfd);
  exit(0);
}
