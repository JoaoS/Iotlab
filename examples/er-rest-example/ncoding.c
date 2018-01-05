


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

	memcpy(Message_list.message_array[Message_list.count_payloads].message_value,incomingPayload, len);
	Message_list.message_array[Message_list.count_payloads].mid = mid;
	PRINTF("mid=%u valor=%c%c\n",Message_list.message_array[Message_list.count_payloads].mid,Message_list.message_array[Message_list.count_payloads].message_value[Message_list.count_payloads],Message_list.message_array[0].message_value[1]);
	Message_list.count_payloads+=1;
}

/*network code one message*/
void code_and_send()



