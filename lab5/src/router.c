#include "ni.h"
#include "router.h"
#include <time.h>
#include <sys/timerfd.h>
#define MAX(a,b) ((a) > (b) ? a : b)
/* Thanks to Prof. Rao for this file. */

int main (int argc, char **argv)
{
    if (argc != 5){
      printf("Usage: ./router <router id> <ni hostname> <ni UDP port> <router UDP port>\n");
      return EXIT_FAILURE;
    }
    
    int router_id, niport, routerport, sockfd;
    char *nihostname, *LogFileName;
    struct sockaddr_in routeraddr;
    FILE *LogFile;
    fd_set rfds;
    struct timeval timeout;

    timeout.tv_sec =  UPDATE_INTERVAL;
    timeout.tv_usec = 0;
    
    /* Store command line arguments */
    router_id = atoi(argv[1]);
    nihostname = argv[2];
    niport = atoi(argv[3]);
    routerport = atoi(argv[4]);

    /* Create and bind the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        printf("Failed to create socket.\n\n");
    bzero((char *) &routeraddr, sizeof(routeraddr));
    routeraddr.sin_port = htons(routerport);
    routeraddr.sin_family = AF_INET;
    routeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&routeraddr, sizeof(routeraddr));

	struct sockaddr_in ni_addr;
 	bzero((char *)&ni_addr, sizeof(ni_addr));
	ni_addr.sin_family = AF_INET;
	inet_aton(nihostname , &ni_addr.sin_addr);
	ni_addr.sin_port = htons((unsigned short)niport);

    /* Code to send INIT_REQUEST goes here */
	struct pkt_INIT_REQUEST request;
	request.router_id = htonl(router_id);
	if (sendto(sockfd, (struct pkt_INIT_REQUEST *)&request, sizeof(request), 0, (struct sockaddr *)&ni_addr, sizeof(ni_addr)) < 0) {
    		printf("\nerror\n");
    		return -1;
   	 }
    /* Code to receive and handle INIT_RESPONSE goes here */
	struct pkt_INIT_RESPONSE response;
    	if (recvfrom(sockfd, (struct pkt_INIT_RESPONSE *)&response, sizeof(response), 0, NULL, NULL) < 0) {
		printf("\nerror\n");
		return -1;
	}
    /* Code to initialize the routing table goes here */
ntoh_pkt_INIT_RESPONSE(&response);
InitRoutingTbl(&response, router_id);


    /* Create and clear the LogFile */
    LogFileName = calloc(100, sizeof(char));
    sprintf(LogFileName, "router%d.log", router_id);
    LogFile = fopen(LogFileName, "w");
    fclose(LogFile);

    /* Main Loop: */
    /* On receiving a RT_UPDATE packet, UpdateRoutes is called to modify the routing table according to the protocol.  
       If the UPDATE_INTERVAL timer expires, first ConvertTabletoPkt is called and a RT_UPDATE is sent to all the neighbors. 
       Second, we check to see if any neighbor has not sent a RT_UPDATE for FAILURE_DETECTION seconds, i.e., (3*UPDATE_INTERVAL). 
       All such neighbors are marked as dead, and UninstallRoutesOnNbrDeath is called to check for routes with the dead nbr 
       as next hop and change their costs to INFINITY. */
  struct pkt_RT_UPDATE updatepacket;
  struct pkt_RT_UPDATE receviedpacket;
int update = 0;
int timeout_fd[response.no_nbr];
struct itimerspec timer[response.no_nbr];
int timeout_status[response.no_nbr];
int i;
int max;
for (i = 0; i < response.no_nbr; i++) {
		timer[i].it_value.tv_sec = FAILURE_DETECTION;
		timer[i].it_value.tv_nsec = 0;
		timer[i].it_interval.tv_sec = 0;
		timer[i].it_interval.tv_nsec = 0;
		timeout_fd[i] = timerfd_create(CLOCK_REALTIME, 0);
		timerfd_settime(timeout_fd[i], 0, &timer[i], NULL);
		max = MAX(max, timeout_fd[i]);
		timeout_status[i] = 0;
	}
 socklen_t serverlen =sizeof(ni_addr);
    while (1)
    {
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);
	for (i = 0; i < response.no_nbr; i++) {
			FD_SET(timeout_fd[i], &rfds);
		}
	max = MAX(max, sockfd);
	select(max +1, &rfds, NULL, NULL, &timeout);
		
        /* Router has received a packet. Update the routing table.
         */
        if (FD_ISSET(sockfd, &rfds))
	{
		recvfrom(sockfd, (struct pkt_RT_UPDATE *)&receviedpacket, sizeof(receviedpacket), 0, (struct sockaddr *) &ni_addr, &serverlen );	
		ntoh_pkt_RT_UPDATE(&receviedpacket);
		int i=0;
		for(i=0; i< response.no_nbr; i++){
			if(receviedpacket.sender_id == response.nbrcost[i].nbr)
				break;
		}
		int cost = response.nbrcost[i].cost;
		if(UpdateRoutes(&receviedpacket, cost, router_id) == 1){
			PrintRoutes(LogFile, router_id);
			update++;
      		}
		timer[i].it_value.tv_sec = FAILURE_DETECTION;
		timer[i].it_value.tv_nsec = 0;
		timerfd_settime(timeout_fd[i], 0, &timer[i], NULL);
		timeout_status[i] = 0;



	 } /* FD_ISSET */

        /* There was no packet received but a timeout has occurred so send an update. 
         */
        else
        {
            	for(i = 0; i < response.no_nbr; i++){
			ConvertTabletoPkt(&updatepacket, router_id); 
			updatepacket.dest_id = (response.nbrcost)[i].nbr;
			hton_pkt_RT_UPDATE(&updatepacket);
			sendto(sockfd, &updatepacket, sizeof(updatepacket), 0, (struct sockaddr *) &ni_addr, sizeof(ni_addr));
      		} 
		for (i = 0; i < response.no_nbr; i++) {
			if(FD_ISSET(timeout_fd[i], &rfds)) {
				if (timeout_status[i] == 0) {
					UninstallRoutesOnNbrDeath(response.nbrcost[i].nbr);
					PrintRoutes(LogFile, router_id);					
					fflush(LogFile);
				}
				timeout_status[i] = 1;
			}
		}	






        } /* else */
    } /* while */
    
	fclose(LogFile);
	
    close(sockfd);
    return 0;
} /* main */


