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
static int seed = 10;
static uint8_t temperature = 0;
static uint8_t rmid=0;
static int len=0;
static s_message_t * buff_packet=NULL;

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  ++event_counter;
  /*
   * For minimal complexity, request query and options should be ignored for GET on observable resources.
   * Otherwise the requests must be stored with the observer list and passed by REST.notify_subscribers().
   * This would be a TODO in the corresponding files in contiki/apps/erbium/!
   */
  //REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  //REST.set_header_max_age(response, res_push.periodic->period / CLOCK_SECOND);
  int i=0;
  static uint8_t packet[COAP_MAX_PACKET_SIZE + 1];
  static int mid_pointer; // to save pointer to buffer to copy mid

  temperature = 1+ random_rand() % 35;
  REST.set_header_etag(response, (uint8_t *)&seed, 1);/*this signals the message has been coded*/
  PRINTF("Temperature = %u\n", temperature);

  len=snprintf((char *)buffer, 10, "%u", temperature);
  mid_pointer=len;

  #if AGGREGATION
  //append every message in the buffer
  for ( i = 0; i < get_coded_len(); ++i){
    printf("yo\n");
    buff_packet = (s_message_t *) get_last_message();

    rmid =(buff_packet->mid & 0xFF00 ) >>8;
    //snprintf uses number of characters so each number is 1 byte; do not use, because human readable uses extra space
    memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
    mid_pointer++;
    rmid = buff_packet->mid & 0x00FF;
    memcpy(buffer+(mid_pointer*sizeof(uint8_t)), &rmid  ,  sizeof(uint8_t));/*apend every mid*/
    mid_pointer++;

    //copiar payload
    memcpy(buffer+(mid_pointer*sizeof(uint8_t)), buff_packet->data, buff_packet->data_len*sizeof(uint8_t));/*apend every mid*/
    mid_pointer+=buff_packet->data_len ;
    //printf("data len=%u\n",buff_packet->data_len );

  }
  #endif
  if (i==0)//for empty buffer
  {
    i=1;
  }
  REST.set_response_payload(response, buffer,  mid_pointer*sizeof(uint8_t));
  //printf("VERY LONG EVENT %lu\n",event_counter );
  /* The REST.subscription_handler() will be called for observable resources by the REST framework. */
 
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
