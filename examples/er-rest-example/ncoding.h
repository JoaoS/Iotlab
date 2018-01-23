/**
 * \file
 *      message storing, message aggregation and network coding
 * \author
 *		João Subtil <jsubtil@student.dei.uc.pt>
 *		
 */


/*create struct to store message value and ID for later coding*/


typedef struct {
	uint8_t message_value[2];
	uint16_t mid;
}singleMessage;

/*array of payloads to send*/
typedef struct{
	int count_payloads;	
	singleMessage message_array[MAX_N_PAYLOADS];	
}aggPayloads;


process_event_t coding_event; /*event to signal a coded message is ready to be sent*/

/*functions*/

/*add payload to buffer*/
void add_payload(uint8_t *incomingPayload, uint16_t mid,uint8_t len );
