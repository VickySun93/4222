#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include<sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
int main(int argc, char *argv[])
{
//set up socket 
	struct sockaddr_in myaddr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd  < 0) { 
		return 0; 
	}
	myaddr.sin_family = AF_INET;
   	myaddr.sin_port = htons( atoi(argv[2]) );// This line of code is used to allow OS assign a port number
	myaddr.sin_addr.s_addr = inet_addr(argv[1]);
	memset(myaddr.sin_zero, '\0', sizeof myaddr.sin_zero);  
	char sendbuf[128];
	memset(sendbuf, 0, sizeof(sendbuf));

//send to proxy server
	sprintf(sendbuf,"Server %s %s", argv[3],argv[4]);
	sendbuf[strlen(sendbuf)] = '\0';	
	while(1){	
		int len = sendto(fd , sendbuf , strlen(sendbuf), 0, (struct sockaddr *)&myaddr,sizeof(myaddr)  );
		if (len > 0) {
			break;
		}
		
	}
//reveived from proxy server
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);
	char recvbuf[1024];
	int recvlen = recvfrom(fd , recvbuf, 1024, 0, (struct sockaddr *)&remaddr, &addrlen);
	if(recvlen > 0){
		fprintf(stdout, "%s\n",recvbuf);

	}

	return 0;

}
