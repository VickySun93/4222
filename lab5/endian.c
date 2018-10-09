  /*
   *  Thanks to Prof. Rao for this file.
   *
   *  File Name: endian.c
   *
   *  Purpose: Defines routines that are useful in endian conversions 
   *
   */


#include "ni.h"

/*
 *  This function converts struct pkt_INIT_RESPONSE
 *  from network to host byte order.
 */
void ntoh_pkt_INIT_RESPONSE (struct pkt_INIT_RESPONSE *ni_resp) {

	int i;

	ni_resp->no_nbr = ntohl(ni_resp->no_nbr);

	for(i=0;i<(ni_resp->no_nbr);i++) {
		(ni_resp->nbrcost[i]).nbr = ntohl((ni_resp->nbrcost[i]).nbr);
		(ni_resp->nbrcost[i]).cost = ntohl((ni_resp->nbrcost[i]).cost);
	}
} /* ntoh_pkt_INIT_RESPONSE */

/*
 *  This function converts struct pkt_RT_UPDATE
 *  from host to network byte order.
 */
void hton_pkt_RT_UPDATE (struct pkt_RT_UPDATE *send_upd) {

	  int i;
	  send_upd->sender_id = htonl (send_upd->sender_id);
	  send_upd->dest_id = htonl (send_upd->dest_id);

	  for (i = 0; i < send_upd->no_routes; i++)
	  {
	    (send_upd->route[i]).dest_id = htonl ((send_upd->route[i]).dest_id);
	    (send_upd->route[i]).next_hop = htonl ((send_upd->route[i]).next_hop);
	    (send_upd->route[i]).cost = htonl ((send_upd->route[i]).cost);
	  }
	  send_upd->no_routes = htonl (send_upd->no_routes);
} /* hton_pkt_RT_UPDATE */

/*
 *  This function converts struct pkt_RT_UPDATE
 *  from network to host byte order.
 */
void ntoh_pkt_RT_UPDATE (struct pkt_RT_UPDATE *recv_upd) {

	  int i = 0;
	  recv_upd->sender_id = ntohl (recv_upd->sender_id);
	  recv_upd->dest_id = ntohl (recv_upd->dest_id);
	  recv_upd->no_routes = ntohl (recv_upd->no_routes);

	  for (i = 0; i < recv_upd->no_routes; i++)
	  {
	    (recv_upd->route[i]).dest_id = ntohl ((recv_upd->route[i]).dest_id);
	    (recv_upd->route[i]).next_hop = ntohl ((recv_upd->route[i]).next_hop);
	    (recv_upd->route[i]).cost = ntohl ((recv_upd->route[i]).cost);
	  }

} /* ntoh_pkt_RT_UPDATE */

