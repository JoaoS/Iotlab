


#include <stdio.h>
#include <string.h>
#include "ncoding.h"
#include "er-coap-observe.h"
#include "er-coap-observe.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void trigger_message(int num);
void free_two_data(void);

MEMB(coded_memb, s_message_t, MAX_CODED_PAYLOADS);
LIST(coded_list); /*points to saved packets*/
int local_loss=0;  //for the lost coded messages
int sent_coded=0;
static int totalcounter=1;
int losses=0;
s_message_t * get_message_pop(void);

/*add this data to the list of packets to process*/
void add_payload(uint8_t *incomingPayload, uint16_t mid, uint8_t len, uip_ipaddr_t * destaddr, uint16_t dport, coap_packet_t * coap_pt ){
	
	s_message_t * message = memb_alloc(&coded_memb);
	totalcounter++;
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

}

/*auxilary functions for aggregation*/
s_message_t * get_message_pop(void){
	return list_pop(coded_list);

}

int get_coded_len(void){
	return list_length(coded_list);
}

void trigger_message(int num){
	if(list_length(coded_list) == 3){//signal for coded message
		losses=num;
		//printf("yoyoyo\n");
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

  int messages_num=0; //number of messages in the list
  int cycles=0;
  s_message_t * mlist=NULL;  
  int counter=0;
  int will_discard=0;
  if (obs)
  {
  	/*check for number of messages and cycle in groups of two
  	for (mlist = (s_message_t *)list_head(coded_list); mlist ;mlist=mlist->next){
  		messages_num++;
  	}*/
	//ALSO DISCARD MESSAGES THAT ARE CODED
	#if GILBERT_ELLIOT_DISCARDER
	will_discard=discard_engine(0); /*return 1 if the packet is to be discarded*/
	#endif
	if (will_discard==1 && losses == 1){
		local_loss=local_loss+1;
		//printf("LOCAL LOSS SENDING FUNCTION =%d\n",local_loss );
		free_two_data();  	//check for number of messages and cycle in groups of two
		return;
	}
	if(!will_discard && losses == 1) {/*negate because 1 equals packet discarded*/
  	  //the destination port is the same as the first saved message/*
	  if((transaction = coap_new_transaction(destination->mid, &obs->addr, obs->port))) {
	    // prepare response 
	    notification->mid = transaction->mid;
		create_xor(notification,transaction->packet + COAP_MAX_HEADER_SIZE, REST_MAX_CHUNK_SIZE);
	    if(notification->code < BAD_REQUEST_4_00) {
	        coap_set_header_observe(notification, destination->observe);
	    }
	    //printf("send message mid=%u, OBSERVE=%ld\n",destination->mid,destination->observe );
	    if (destination->token_len > 0)
	    {
	    	PRINTF("COAP token len=%u, token=%02x%02x%02x%02x \n",destination->token_len,destination->token[0],destination->token[1],destination->token[2],destination->token[3]);
	    	coap_set_token(notification, destination->token, destination->token_len);
	    }
	    transaction->packet_len = coap_serialize_message(notification, transaction->packet);
	    sent_coded=sent_coded+1;
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
	      */
	    }
	}
	free_two_data();
 	PRINTF("free=%d\n",memb_numfree(&coded_memb));
}

void create_xor(void *response, uint8_t *buffer, uint16_t preferred_size){

	static int packetnumber;	/*count payloads to signal it in the etag*/
	static int mid_pointer; // to save pointer to buffer to copy mid
	static s_message_t * destination=NULL;
	uint16_t xoring_buffer[2];
	uint8_t rmid=0;
	packetnumber=mid_pointer=0;
	memset(xoring_buffer, 0, sizeof(xoring_buffer));
	int i=0;
	static uint16_t temp=0; //for temporary payloads due to array
	

	static int len=0;
	static int extra_packet_len=0; /*we are goint to put the number of extra bytes in the etag, only one payload is suposed to be aggregated, */
	static s_message_t * buff_packet=NULL;

	len=get_coded_len();
	extra_packet_len=0;
    for ( i = 0; i < 2; ++i){

        buff_packet = (s_message_t *) get_message_pop(); //pop and return all messages
        extra_packet_len = 2 + extra_packet_len + buff_packet->data_len; //the mid space + the len in bytes of temperature
        
        rmid =(buff_packet->mid & 0xFF00 ) >>8;
        //snprintf uses number of characters so each number is 1 byte; do not use, because human readable uses extra space
        memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
        mid_pointer++;
        rmid = buff_packet->mid & 0x00FF;
        memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
        mid_pointer++;

        /***************************ALSO NECESSARY TO COPY THE AGGREGATED MIDS AND PAYLOADS**********************/
        /*AGGREGATED MIDS*/
        memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &(buff_packet->data[2]),sizeof(uint8_t));/*apend every mid*/
        mid_pointer++;
        memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &(buff_packet->data[3]),sizeof(uint8_t));/*apend every mid*/
        mid_pointer++;

        /*Now xor all payloads, these will be added at the end*/
		temp=((uint16_t) buff_packet->data[0] << 8) | buff_packet->data[1];
        xoring_buffer[0] ^= temp;
        //printf("temp=%04x    xoring buffer=%04x\n",temp, xoring_buffer );
        temp=((uint16_t) buff_packet->data[4] << 8) | buff_packet->data[5];

        xoring_buffer[1]^= temp;
        //printf("temp=%04x    xoring buffer=%04x\n",temp, xoring_buffer );
              //printf("xoring buffer=%04x\n",xoring_buffer );
        free_poped_memb(buff_packet);//free the alocated memory
      }
      if(i==0)   //NO MESSAGES
      {
        i=1;
      }
      rmid = (xoring_buffer[0] & 0xFF00 ) >>8;
      memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid ,sizeof(uint8_t));/*apend every mid*/
      mid_pointer++;
      rmid = xoring_buffer[0] & 0x00FF;
      memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid ,sizeof(uint8_t));/*apend every mid*/
      mid_pointer++;
      rmid = (xoring_buffer[1] & 0xFF00 ) >>8;
      memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid ,sizeof(uint8_t));/*apend every mid*/
      mid_pointer++;
      rmid = xoring_buffer[1] & 0x00FF;
      memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid ,sizeof(uint8_t));/*apend every mid*/
      mid_pointer++;


      printf("extra_packet_len=%d\n",extra_packet_len );
      printf("final=xoring buffer=%04x\n",xoring_buffer );



      REST.set_header_etag(response, (uint8_t *)&mid_pointer, 1);
      REST.set_response_payload(response, buffer,  mid_pointer*sizeof(uint8_t));

}

void free_data(void){
	s_message_t * mlist=NULL;  
 	for(mlist = (s_message_t *)list_head(coded_list); mlist ;mlist=mlist->next) {
    remove_element(mlist);
  }
}

//message was poped, free memory
void free_poped_memb(s_message_t * o){
	memb_free(&coded_memb, o);
}

void remove_element(s_message_t * o){
	
	//PRINTF("dealoc=%d, \n",memb_free(&coded_memb, list_pop(coded_list))); 
	memb_free(&coded_memb, list_pop(coded_list));
	//memb_free(&coded_list, o);
  //list_remove(coded_list,o); /*has to be pop so cycle does not loop*/  
}


/**
*free data just from two messages, for the coding process
*/
void free_two_data(void){
  s_message_t * mlist=NULL; 
  int i=0; 
  PRINTF("clearing coded \n");
  for(mlist = (s_message_t *)list_head(coded_list); mlist ;mlist=mlist->next) {
    
    if (i==TRIGGERPACKETS)
    {
      break;
    }
    remove_element(mlist);
    i++;
  }
}