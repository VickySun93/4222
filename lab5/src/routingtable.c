#include "ni.h"
#include "router.h"

/* Thanks to Prof. Rao for this file. */

/* The following two global variables are defined and used only in routingtable.c for convenience. */

struct route_entry routingTable [MAX_ROUTERS];
/* This is the routing table that contains all the route entries. It is initialized by InitRoutingTbl() and updated
   by UpdateRoutes().*/

int NumRoutes;
/*  This variable denotes the number of routes in the routing table. 
    It is initialized by InitRoutingTbl() on receiving the INIT_RESPONSE and updated by UpdateRoutes() 
    when the routingTable changes.*/


/**
 * This function initializes the routing table with the neighbor information received from the network initializer.
 * It adds a self-route (route to the same router) in the routing table.
 */
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID)
{	int i;
	NumRoutes = InitResponse->no_nbr+1;
	for(i=0; i< NumRoutes; i++){
		routingTable[i].dest_id = InitResponse->nbrcost[i].nbr;
		routingTable[i].next_hop = InitResponse->nbrcost[i].nbr;
		routingTable[i].cost =  InitResponse->nbrcost[i].cost;
	}
 	routingTable[i].dest_id = myID;
  	routingTable[i].next_hop = myID;
  	routingTable[i].cost = 0;
} /* InitRoutingTbl */

/**
 * This function is invoked on receiving route update message from a neighbor. 
 * It installs in the routing table new routes that are previously unknown. 
 * For known routes, it finds the shortest path and updates the routing table if necessary. 
 * It also implements the forced update and split horizon rules.
 * It returns 1 if the routing table changed and 0 otherwise.
 */


int UpdateRoutes (struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID)
{
int change =0;
int i,j;
//check next hop, itself to reach c? next hop is A with higher cost to rech C.
	for(i =0; i< RecvdUpdatePacket->no_routes; i++){
		struct route_entry entry = RecvdUpdatePacket->route[i];
		for(j = 0; j< NumRoutes; j++){
			if(routingTable[j].dest_id == entry.dest_id ){


				if(routingTable[j].next_hop == RecvdUpdatePacket->sender_id){
					
					if((costToNbr+ entry.cost) != routingTable[j].cost){
						if (routingTable[j].cost == INFINITY && costToNbr + entry.cost >= INFINITY) {
  							routingTable[j].cost = INFINITY;
  							change = 0;
  						}
  						else if (costToNbr + entry.cost >= INFINITY) {
  							routingTable[j].cost= INFINITY;
  							change = 1;
  						}
  						else {
  							routingTable[j].cost= costToNbr + entry.cost;
  							change = 1;		
  						}

					}

				}else if(entry.next_hop!=myID){
					if(costToNbr + entry.cost < routingTable[j].cost){
						routingTable[j].next_hop =RecvdUpdatePacket->sender_id;
						if (routingTable[j].cost == INFINITY && costToNbr + entry.cost >= INFINITY) {
  							routingTable[j].cost = INFINITY;
  							change = 0;
  						}
  						else if (costToNbr + entry.cost >= INFINITY) {
  							routingTable[j].cost= INFINITY;
  							change = 1;
  						}
  						else {
  							routingTable[j].cost= costToNbr + entry.cost;
  							change = 1;		
  						}
					}

				}



				break;
			}
		}
		if( j == NumRoutes ) {
  			routingTable[j].dest_id = entry.dest_id;
			routingTable[j].next_hop = RecvdUpdatePacket->sender_id;
			routingTable[j].cost = entry.cost + costToNbr;
			NumRoutes++;
			change = 1;

  		}
	}
   return change;
} /* UpdateRoutes */

/**
 * This function is invoked on timer expiry to advertise the routes to neighbors. 
 * It fills the routing table information into a struct pkt_RT_UPDATE, a pointer to which 
 * is passed as an input argument.
 */

void ConvertTabletoPkt (struct pkt_RT_UPDATE *UpdatePacketToSend, int myID)
{int i;

	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend -> no_routes = NumRoutes;
  	for(i = 0; i < NumRoutes; i++){
    		UpdatePacketToSend -> route[i].dest_id = routingTable[i].dest_id;
    		UpdatePacketToSend -> route[i].next_hop = routingTable[i].next_hop;
    		UpdatePacketToSend -> route[i].cost = routingTable[i].cost;
  	}
} /* ConvertTabletoPkt */

/**
 * This function is invoked whenever the routing table changes.
 * It prints the current routing table information to a log file, a pointer to which 
 * is passed as an input argument. The handout gives the file format.
 */
void PrintRoutes (FILE* Logfile, int myID)
{	int i;
  	for(i = 0; i < NumRoutes; i++){
    		fprintf(Logfile, "R%d -> R%d: R%d, %d\n", myID, routingTable[i].dest_id, routingTable[i].next_hop, routingTable[i].cost);
    		fflush(Logfile);
  	}
} /* PrintRoutes */

/**
 * This function is invoked on detecting an inactive link to a neighbor (dead nbr). 
 * It checks all the routes that use the dead nbr as the next hop and changes the cost to INFINITY.
 */
void UninstallRoutesOnNbrDeath (int DeadNbr)
{ int i;
  for(i = 0; i < NumRoutes; i++){
    if(routingTable[i].next_hop == DeadNbr){
      routingTable[i].cost = INFINITY;
    }
  }
} /* UninstallRoutesOnNbrDeath */


 
