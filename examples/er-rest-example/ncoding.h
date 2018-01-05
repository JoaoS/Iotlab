/**
 * \file
 *      message storing, message aggregation and network coding
 * \author
 *		João Subtil <jsubtil@student.dei.uc.pt>
 *		
 */



/*create struct to store message value and ID for later coding*/
typedef struct{
	uint8_t message_value[2];
	uint16_t mid;
}singleMessage;

/*array of payloads to send*/
typedef struct{
	int count_payloads;	
	singleMessage message_array[MAX_N_PAYLOADS];	
}aggPayloads;

static aggPayloads Message_list={0};




/*functions*/

void add_payload(uint8_t *incomingPayload, uint16_t mid,uint8_t len );
