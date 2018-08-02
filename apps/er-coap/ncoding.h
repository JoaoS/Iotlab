/**
 * \file
 *      message storing, message aggregation and network coding
 * \author
 *		João Subtil <jsubtil@student.dei.uc.pt>
 *		
 */

#ifndef COAP_CODING_H_
#define COAP_CODING_H_

#include "er-coap.h"

#include "er-coap-transactions.h"
//#include "rest-engine.h"

/*create struct to store message value and ID for later coding*/
/*TODO: in the future url might be added and create a struct element list for each resource instead of simple struct list*/
typedef struct s_message{
	struct s_message *next;   /* for LIST */
	uip_ipaddr_t addr; /*ip address of observer, it will be the same for now, only one group in the network*/
	uint16_t port;	//the port is what dinstinguishes the coap requests, it might be different but use just one
	uint8_t data[7];	// 2 plus null terminator(3) + 4 bytes when using aggregation mid+temperature
	uint8_t data_len; // to be used in the mask xor process
	uint16_t mid;
	int32_t observe;
  	uint8_t token_len;
  	uint8_t token[COAP_TOKEN_LEN];
  	int discarded;

}s_message_t;

/*array of payloads to send*/

process_event_t coding_event; /*event to signal a coded message is ready to be sent*/


/*functions*/

/*add payload to buffer*/
void add_payload(uint8_t *incomingPayload, uint16_t mid, uint8_t len, uip_ipaddr_t * destaddr, uint16_t dport, coap_packet_t * coap_pt  );
void send_coded(resource_t *resource);
void create_xor(void *response, uint8_t *buffer, uint16_t preferred_size);
void remove_element(s_message_t * o);
void free_data(void);

#endif