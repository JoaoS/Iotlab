/**
 * \file
 *      message storing, message aggregation and network coding
 * \author
 *		João Subtil <jsubtil@student.dei.uc.pt>
 *		
 */

/*create struct to store message value and ID for later coding*/
#include "er-coap.h"
#include "er-coap-transactions.h"

/*TODO: in the future url might be added and create a struct element list for each resource instead of simple struct list*/
typedef struct s_message{
	struct s_message *next;   /* for LIST */
	
	uip_ipaddr_t addr; /*ip address of observer, it will be the same for now, only one group in the network*/
	uint16_t port;	//the port is what dinstinguishes the coap requests, it might be different but use just one
	uint8_t data[3];	// 2 plus null terminator
	uint8_t data_len; // to be used in the mask xor process
	uint16_t mid;
	
}s_message_t;

/*array of payloads to send*/

process_event_t coding_event; /*event to signal a coded message is ready to be sent*/


/*functions*/

/*add payload to buffer*/
void add_payload(uint8_t *incomingPayload, uint16_t mid, uint8_t len, uip_ipaddr_t * destaddr, uint16_t dport );
void send_coded(resource_t *resource);
void create_xor(void *response, uint8_t *buffer, uint16_t preferred_size);
void remove_element(s_message_t * o);

void free_data(void);
