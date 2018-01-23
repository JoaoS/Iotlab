


#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "ncoding.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static aggPayloads Message_list={0};

PROCESS_NAME(er_example_server);
static char msg[]="oi";


/*The first call to this function is when a packet is produced, so it initializes at 0*/
void reset_payloads(){
	int i;
	for(i=0;i<Message_list.count_payloads; i=i+1){
		Message_list.message_array[i].mid=0;
	}
	Message_list.count_payloads=0;
	
	#if DEBUG_DENSENET
		printf("Parsing: Reseting aggregated payloads \n");
	#endif
}

/**/
void add_payload(uint8_t *incomingPayload, uint16_t mid, uint8_t len ){

	printf("len payload (ncoding)=%u\n",len );

	memcpy(Message_list.message_array[Message_list.count_payloads].message_value,incomingPayload, len);
	Message_list.message_array[Message_list.count_payloads].mid = mid;
	

	#if DEBUG
	uint8_t * number_value = &Message_list.message_array[Message_list.count_payloads].message_value;
	PRINTF("len pacote= %u, mid=%u valor= %c %c\n",len,Message_list.message_array[Message_list.count_payloads].mid,      
		number_value[0],number_value[1]);
	printf("size pacote=%u\n",sizeof(incomingPayload) );
	#endif

	Message_list.count_payloads+=1;
	/*base cenario 2 packets, send the coded message after*/
	if (Message_list.count_payloads == 2 )
	{
		printf("nr mensagens =%d\n",Message_list.count_payloads );

		/*post asynchronous event to send coded message*/
      	process_post(&er_example_server,coding_event, msg);
	}
}

/*network code one message*/
/*void code_and_send()*/



