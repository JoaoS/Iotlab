/**
 * \file
 *      network coding message reource
 * \author
 *      João Subtil <jsubtil@dei.uc.pt>
 *    
 */

#include <string.h>
#include <stdio.h>
#include "rest-engine.h"
#include "er-coap.h"
#include "random.h"

#if AGGREGATION
#include "ncoding.h"
#endif


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);


PERIODIC_RESOURCE(res_coded,
                  "title=\"Periodic demo\";obs",
                  res_get_handler,
                  NULL,
                  NULL,
                  NULL,
                  SEND_MESSAGE_INTERVAL * CLOCK_SECOND,
                  res_periodic_handler);

/*
 * Use local resource state that is accessed by res_get_handler() and altered by res_periodic_handler() or PUT or POST.
 */
static uint16_t event_counter = 0;
static int seed = 1;
static uint8_t temperature = 0;

#if AGGREGATION
  static uint8_t rmid=0;
  static int len=0;
  static int extra_packet_len=0; /*we are goint to put the number of extra bytes in the etag, only one payload is suposed to be aggregated, */
  static s_message_t * buff_packet=NULL;
#endif
static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  ++event_counter;

  temperature = 10+ random_rand() % 40;

  #if AGGREGATION
    static int i=0;
    static int mid_pointer; // to save pointer to buffer to copy mid
    if (rank_level > 1)//agrega a mensagem armazenada
    {   
        //REST.set_header_etag(response, (uint8_t *)&seed, 1);/*this signals the message has been coded*/
        PRINTF("Temperature = %u\n", temperature);
        mid_pointer=snprintf((char *)buffer, 10, "%u", temperature);;
        extra_packet_len=0;
          //append every message in the buffer
        len=get_coded_len();
        for ( i = 0; i < len; ++i){

          buff_packet = (s_message_t *) get_message_pop(); //pop and return all messages
          extra_packet_len = 2 + extra_packet_len + buff_packet->data_len; //the mid space + the len in bytes of temperature
          
          rmid =(buff_packet->mid & 0xFF00 ) >>8;
          //snprintf uses number of characters so each number is 1 byte; do not use, because human readable uses extra space
          memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
          mid_pointer++;
          rmid = buff_packet->mid & 0x00FF;
          memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
          mid_pointer++;
        
          memcpy(buffer+(mid_pointer*sizeof(uint8_t)), buff_packet->data, buff_packet->data_len*sizeof(uint8_t));/*apend every mid*/
          mid_pointer+=buff_packet->data_len ;
          free_poped_memb(buff_packet);//free the alocated memory
        }
        //NO MESSAGES TO AGGREGATE, just send temp
        if(i==0){
          i=1;}
        len=len+1;
        REST.set_header_etag(response, (uint8_t *)&len, 1);
        REST.set_response_payload(response, buffer,  mid_pointer*sizeof(uint8_t));
           // printf("temperature=%d\n",temperature );

    }
    #endif     

   #if !AGGREGATION
    REST.set_header_etag(response, (uint8_t *)&seed, 1);/*this signals the message has been coded*/
    REST.set_response_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%u", temperature));
    //printf("temperature=%d\n",temperature );
  #endif
  //printf("tamanho lista =%d\n",get_coded_len() );
      
}


/*
 * Additionally, a handler function named [resource name]_handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
  /* Do a periodic task here, e.g., sampling a sensor. */
  //++event_counter;

  /* Usually a condition is defined under with subscribers are notified, e.g., large enough delta in sensor reading. */
  if(1) {
    /* Notify the registered observers which will trigger the res_get_handler to create the response. */
    REST.notify_subscribers(&res_coded);
  }
}
