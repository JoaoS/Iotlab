


#include <stdio.h>
#include <string.h>
#include "ncoding.h"
#include "er-coap-observe.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

MEMB(coded_memb, s_message_t, MAX_CODED_PAYLOADS);
LIST(coded_list); /*points to saved packets*/

static int totalcounter=1;

/**/
void add_payload(uint8_t *incomingPayload, uint16_t mid, uint8_t len, uip_ipaddr_t * destaddr, uint16_t dport, coap_packet_t * coap_pt ){
	
	s_message_t * message = memb_alloc(&coded_memb);
	PRINTF("Adding message=%d\n",totalcounter);
	totalcounter++;
	//printf("coap token len=%u, token=%02x%02x%02x%02x \n",coap_pt->token_len,coap_pt->token[0],coap_pt->token[1],coap_pt->token[2],coap_pt->token[3]);
	//printf("observe=%ld\n",coap_pt->observe );
	if (message)
	{
		memcpy(message->data,incomingPayload,len);
		message->data[len] = '\0';
		message->mid = mid;
		message->data_len=len;
		uip_ipaddr_copy(&message->addr, destaddr);
		message->port = dport;
		/*token, token_len, code*/
		message->observe=coap_pt->observe;
		message->token_len=coap_pt->token_len;
		memcpy(message->token, coap_pt->token, coap_pt->token_len);
		message->token[coap_pt->token_len] = '\0';
		list_add(coded_list, message);
	}
	else
		printf("ERROR ALOCATING MEMORY FOR PACKET\n");

	if(list_length(coded_list) % TRIGGERPACKETS == 0){//signal for coded message
		process_post(PROCESS_BROADCAST,coding_event, NULL);
	}
}

/*construct and send a coded message, clean stored data afterwards*/
void send_coded(resource_t *resource){
  
  /* build notification */
  static coap_packet_t notification[1]; /* this way the packet can be treated as pointer as usual */
  static s_message_t * destination=NULL;
  coap_init_message(notification, COAP_TYPE_NON, CONTENT_2_05, 0); //to be sent
  
  coap_transaction_t *transaction = NULL;
  coap_observer_t * obs = (coap_observer_t *)get_observers();

  //for now there is just one subscriber
  destination = (s_message_t *)list_head(coded_list);
  
  if (obs)
  {
  	  //the destination port is the same as the first saved message/*
	  if((transaction = coap_new_transaction(destination->mid, &obs->addr, obs->port))) {
	    // prepare response 
	    notification->mid = transaction->mid;
		create_xor(notification,transaction->packet + COAP_MAX_HEADER_SIZE, REST_MAX_CHUNK_SIZE);
	    if(notification->code < BAD_REQUEST_4_00) {
	        coap_set_header_observe(notification, destination->observe);
	    }
	    printf("send message mid=%u, OBSERVE=%ld\n",destination->mid,destination->observe );
	    if (destination->token_len > 0)
	    {
	    	PRINTF("COAP token len=%u, token=%02x%02x%02x%02x \n",destination->token_len,destination->token[0],destination->token[1],destination->token[2],destination->token[3]);
	    	coap_set_token(notification, destination->token, destination->token_len);
	    }
	    transaction->packet_len = coap_serialize_message(notification, transaction->packet);
	    coap_send_transaction(transaction);
	    }
		/*/
	    if((transaction = coap_new_transaction(coap_get_mid(), &obs->addr, obs->port))) {
	        
	        // update last MID for RST matching 
	        obs->last_mid = transaction->mid;
	        // prepare response 
	        notification->mid = transaction->mid;

	        create_xor(notification,transaction->packet + COAP_MAX_HEADER_SIZE, REST_MAX_CHUNK_SIZE);
		
			if(notification->code < BAD_REQUEST_4_00) {
		        coap_set_header_observe(notification, destination->observe);
		    }
		    coap_set_token(notification, destination->token, destination->token_len);
		    transaction->packet_len = coap_serialize_message(notification, transaction->packet);
	        coap_send_transaction(transaction);
	      }
	      /**/
	}

    //clean buffer
    //printf("cleaning data\n");
	free_data();
 	PRINTF("free=%d\n",memb_numfree(&coded_memb));
}

void create_xor(void *response, uint8_t *buffer, uint16_t preferred_size){

	static int packetnumber;	/*count payloads to signal it in the etag*/
	static int mid_pointer; // to save pointer to buffer to copy mid
	static s_message_t * destination=NULL;
	uint8_t xored_data[2];
	uint8_t rmid=0;
	packetnumber=mid_pointer=0;
	memset(xored_data, 0, sizeof(xored_data));



		//first mid then all payloads xored
	for(destination = (s_message_t *)list_head(coded_list); destination; destination = destination->next) {

		/*i use this method instead of memcpy uint16_t to prevent the change in byte order due to endianess of cpu*/
		rmid	=(destination->mid & 0xFF00 ) >>8;
		//snprintf uses number of characters so each number is 1 byte; do not use, because human readable uses extra space
		memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
		mid_pointer++;
		rmid = destination->mid & 0x00FF;
		memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
		mid_pointer++;
		packetnumber++;	

		if (destination->data_len == 1)//only 1 number is stored in the first array position
		{
			xored_data[0] ^= destination->data[0];  //xor 1 st byte
			xored_data[1] ^= 0;
		}
		else if(destination->data_len == 2){
			xored_data[0] ^= destination->data[0];  //xor 1 st byte
			xored_data[1] ^= destination->data[1];
		}

	}

	//xor payload
	memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &xored_data[0]  ,  sizeof(uint8_t));
	mid_pointer++;
	memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &xored_data[1]  ,  sizeof(uint8_t));
	mid_pointer++;	//update the len of  buffer to send correct size message
	REST.set_header_etag(response, (uint8_t *)&packetnumber, 1); /*indicate number of packets that have been coded*/
	REST.set_response_payload(response, buffer, (mid_pointer)*sizeof(uint8_t));
}

void free_data(void){

	s_message_t * mlist=NULL;  

	PRINTF("clearing coded \n");
 
 	for(mlist = (s_message_t *)list_head(coded_list); mlist ;mlist=mlist->next) {
    remove_element(mlist);
  }
  /*coap_transaction_t *t = NULL;
 
  for(t = (coap_transaction_t *)list_head(transactions_list); t; t = t->next) {
    printf("atempting remove transaction %u: %p\n", t->mid, t);
    remove_trans_element(t);
  }*/
}

void remove_element(s_message_t * o){
	
	//PRINTF("dealoc=%d, \n",memb_free(&coded_memb, list_pop(coded_list))); 
	memb_free(&coded_memb, list_pop(coded_list));
	//memb_free(&coded_list, o);
  //list_remove(coded_list,o); /*has to be pop so cycle does not loop*/  
}