/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         Code for tunnelling uIP packets over the Rime mesh routing module
 *
 * \author  Adam Dunkels <adam@sics.se>\author
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 */


#include "contiki-net.h"
#include "net/ip/uip-split.h"
#include "net/ip/uip-packetqueue.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6.h"
#endif

#if UIP_CONF_IPV6_RPL
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#endif

#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#define UIP_ICMP_BUF ((struct uip_icmp_hdr *)&uip_buf[UIP_LLIPH_LEN + uip_ext_len])
#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_TCP_BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

#ifdef UIP_FALLBACK_INTERFACE
extern struct uip_fallback_interface UIP_FALLBACK_INTERFACE;
#endif

#if UIP_CONF_IPV6_RPL
#include "rpl/rpl.h"
#endif


/*NETWORK_CODING densenet*/
#if COM_ACKS
#include "apps/er-coap/er-coap.h"
#include "apps/rest-engine/rest-engine.h"
#include <stdlib.h>
#include "random.h"
#define REQUEST_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0 , 0x0001)      /* cooja2 */
//#define REQUEST_NODE(ipaddr)   uip_ip6addr(ipaddr, 0x2001, 0x0660, 0x5307, 0x3111, 0, 0, 0 , 0x0001) 
static uip_ipaddr_t server_ipaddr;
static void
print_ipv6_addr(const uip_ipaddr_t *ip_addr) {
    int i;
    for (i = 14; i < 16; i++) {
        printf("%02x", ip_addr->u8[i]);
    }
}
int id_node; /*for cooja id nodes*/
 int total_forwarded=0;
 int total_dropped=0;
 int from_node3=0;
 int from_node4=0;
void print_mid();
int discard_engine(int _node_id);
int loss_array[ELEMENTS]={0};
#endif
/* code ends*/
#if NETWORK_CODING   
void store_msg(void);
#endif



process_event_t tcpip_event;
#if UIP_CONF_ICMP6
process_event_t tcpip_icmp6_event;
#endif /* UIP_CONF_ICMP6 */

/* Periodic check of active connections. */
static struct etimer periodic;

#if NETSTACK_CONF_WITH_IPV6 && UIP_CONF_IPV6_REASSEMBLY
/* Timer for reassembly. */
extern struct etimer uip_reass_timer;
#endif

#if UIP_TCP
/**
 * \internal Structure for holding a TCP port and a process ID.
 */
struct listenport {
  uint16_t port;
  struct process *p;
};

static struct internal_state {
  struct listenport listenports[UIP_LISTENPORTS];
  struct process *p;
} s;
#endif

enum {
  TCP_POLL,
  UDP_POLL,
  PACKET_INPUT
};

/* Called on IP packet output. */
#if NETSTACK_CONF_WITH_IPV6

static uint8_t (* outputfunc)(const uip_lladdr_t *a);

uint8_t
tcpip_output(const uip_lladdr_t *a)
{
  int ret;
  if(outputfunc != NULL) {
    ret = outputfunc(a);
    return ret;
  }
  UIP_LOG("tcpip_output: Use tcpip_set_outputfunc() to set an output function");
  return 0;
}

void
tcpip_set_outputfunc(uint8_t (*f)(const uip_lladdr_t *))
{
  outputfunc = f;
}
#else

static uint8_t (* outputfunc)(void);
uint8_t
tcpip_output(void)
{
  if(outputfunc != NULL) {
    return outputfunc();
  }
  UIP_LOG("tcpip_output: Use tcpip_set_outputfunc() to set an output function");
  return 0;
}

void
tcpip_set_outputfunc(uint8_t (*f)(void))
{
  outputfunc = f;
}
#endif

#if UIP_CONF_IP_FORWARD
unsigned char tcpip_is_forwarding; /* Forwarding right now? */
#endif /* UIP_CONF_IP_FORWARD */

PROCESS(tcpip_process, "TCP/IP stack");

/*---------------------------------------------------------------------------*/
#if UIP_TCP || UIP_CONF_IP_FORWARD
static void
start_periodic_tcp_timer(void)
{
  if(etimer_expired(&periodic)) {
    etimer_restart(&periodic);
  }
}
#endif /* UIP_TCP || UIP_CONF_IP_FORWARD */
/*---------------------------------------------------------------------------*/
static void
check_for_tcp_syn(void)
{
#if UIP_TCP || UIP_CONF_IP_FORWARD
  /* This is a hack that is needed to start the periodic TCP timer if
     an incoming packet contains a SYN: since uIP does not inform the
     application if a SYN arrives, we have no other way of starting
     this timer.  This function is called for every incoming IP packet
     to check for such SYNs. */
#define TCP_SYN 0x02
  if(UIP_IP_BUF->proto == UIP_PROTO_TCP &&
     (UIP_TCP_BUF->flags & TCP_SYN) == TCP_SYN) {
    start_periodic_tcp_timer();
  }
#endif /* UIP_TCP || UIP_CONF_IP_FORWARD */
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  if(uip_len > 0) {

#if UIP_CONF_IP_FORWARD
    tcpip_is_forwarding = 1;
    if(uip_fw_forward() != UIP_FW_LOCAL) {
      tcpip_is_forwarding = 0;
      return;
    }
    tcpip_is_forwarding = 0;
#endif /* UIP_CONF_IP_FORWARD */

    check_for_tcp_syn();
    uip_input();
#if HARDCODED_TOPOLOGY
  static uip_ipaddr_t node7,node8,node9,node10,node11,node12;
  uip_ip6addr(&node7, 0x2001, 0x0660, 0x3207, 0x04c0, 0, 0, 0 , NODE_7_IP);
  uip_ip6addr(&node8, 0x2001, 0x0660, 0x3207, 0x04c0, 0, 0, 0 , NODE_8_IP);
  uip_ip6addr(&node9, 0x2001, 0x0660, 0x3207, 0x04c0, 0, 0, 0 , NODE_9_IP);
  uip_ip6addr(&node10, 0x2001, 0x0660, 0x3207, 0x04c0, 0, 0, 0 , NODE_10_IP);
  uip_ip6addr(&node11, 0x2001, 0x0660, 0x3207, 0x04c0, 0, 0, 0 , NODE_11_IP);
  uip_ip6addr(&node12, 0x2001, 0x0660, 0x3207, 0x04c0, 0, 0, 0 , NODE_12_IP);
  

  //uip_ip6addr(&node3, 0xfd00, 0, 0, 0, 200, 0, 0 , 0x0003);
  //uip_ip6addr(&node4, 0xfd00, 0, 0, 0, 200, 0, 0 , 0x0004);
  /*if (!dis_flag) //debug prints
  {
    
    if( (uip_ip6addr_cmp(&node3,&UIP_IP_BUF->srcipaddr) ) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
      printf("forwarding message\n");
      print_mid(3);
    } 
    if( (uip_ip6addr_cmp(&node4,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
      printf("forwarding message\n");
      print_mid(4);
    } 
  }*/
  #if NETWORK_CODING
        //save message if destination is the external observer given in er-example, if the server ip is fd00::::1 it activates the web browser requests
    if((uip_ip6addr_cmp(&node4,&UIP_IP_BUF->srcipaddr) || uip_ip6addr_cmp(&node3,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){ 
      //printf("TESTE\n");
      store_msg();
    }
#endif
  
  if (dis_flag) // set in er-rest-example
  {
#if GILBERT_ELLIOT_DISCARDER /*here print the retransmission and drop if conditions are met*/
    if( (uip_ip6addr_cmp(&node7,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
      if(discard_engine(7)){
        loss_array[7]=loss_array[7]+1;
      }
    } 
    if( (uip_ip6addr_cmp(&node8,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
        if(discard_engine(8)){
        loss_array[8]=loss_array[8]+1;     
      }
    } 
    if( (uip_ip6addr_cmp(&node9,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
        if(discard_engine(9)){
          loss_array[9]=loss_array[9]+1; 
      }
    } 
    if( (uip_ip6addr_cmp(&node10,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
        if(discard_engine(10)){
          loss_array[10]=loss_array[10]+1; 
      }
    } 
    if( (uip_ip6addr_cmp(&node11,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
        if(discard_engine(11)){
          loss_array[11]=loss_array[11]+1; 
      }
    } 
    if( (uip_ip6addr_cmp(&node12,&UIP_IP_BUF->srcipaddr)) && !uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
        if(discard_engine(12)){
          loss_array[12]=loss_array[12]+1; 
      }
      
    } 
#endif
  }
#endif


    if(uip_len > 0) {
#if UIP_CONF_TCP_SPLIT
      uip_split_output();
#else /* UIP_CONF_TCP_SPLIT */
#if NETSTACK_CONF_WITH_IPV6
      tcpip_ipv6_output();
#else /* NETSTACK_CONF_WITH_IPV6 */
      PRINTF("tcpip packet_input output len %d\n", uip_len);
      tcpip_output();
#endif /* NETSTACK_CONF_WITH_IPV6 */
#endif /* UIP_CONF_TCP_SPLIT */
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_TCP
#if UIP_ACTIVE_OPEN
struct uip_conn *
tcp_connect(const uip_ipaddr_t *ripaddr, uint16_t port, void *appstate)
{
  struct uip_conn *c;

  c = uip_connect(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  c->appstate.p = PROCESS_CURRENT();
  c->appstate.state = appstate;

  tcpip_poll_tcp(c);

  return c;
}
#endif /* UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
void
tcp_unlisten(uint16_t port)
{
  unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == port &&
       l->p == PROCESS_CURRENT()) {
      l->port = 0;
      uip_unlisten(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp_listen(uint16_t port)
{
  unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == 0) {
      l->port = port;
      l->p = PROCESS_CURRENT();
      uip_listen(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp_attach(struct uip_conn *conn,
	   void *appstate)
{
  uip_tcp_appstate_t *s;

  s = &conn->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;
}

#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
void
udp_attach(struct uip_udp_conn *conn,
	   void *appstate)
{
  uip_udp_appstate_t *s;

  s = &conn->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;
}
/*---------------------------------------------------------------------------*/
struct uip_udp_conn *
udp_new(const uip_ipaddr_t *ripaddr, uint16_t port, void *appstate)
{
  struct uip_udp_conn *c;
  uip_udp_appstate_t *s;

  c = uip_udp_new(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  s = &c->appstate;
  s->p = PROCESS_CURRENT();
  s->state = appstate;

  return c;
}
/*---------------------------------------------------------------------------*/
struct uip_udp_conn *
udp_broadcast_new(uint16_t port, void *appstate)
{
  uip_ipaddr_t addr;
  struct uip_udp_conn *conn;

#if NETSTACK_CONF_WITH_IPV6
  uip_create_linklocal_allnodes_mcast(&addr);
#else
  uip_ipaddr(&addr, 255,255,255,255);
#endif /* NETSTACK_CONF_WITH_IPV6 */
  conn = udp_new(&addr, port, appstate);
  if(conn != NULL) {
    udp_bind(conn, port);
  }
  return conn;
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ICMP6
uint8_t
icmp6_new(void *appstate) {
  if(uip_icmp6_conns.appstate.p == PROCESS_NONE) {
    uip_icmp6_conns.appstate.p = PROCESS_CURRENT();
    uip_icmp6_conns.appstate.state = appstate;
    return 0;
  }
  return 1;
}

void
tcpip_icmp6_call(uint8_t type)
{
  if(uip_icmp6_conns.appstate.p != PROCESS_NONE) {
    /* XXX: This is a hack that needs to be updated. Passing a pointer (&type)
       like this only works with process_post_synch. */
    process_post_synch(uip_icmp6_conns.appstate.p, tcpip_icmp6_event, &type);
  }
  return;
}
#endif /* UIP_CONF_ICMP6 */
/*---------------------------------------------------------------------------*/
static void
eventhandler(process_event_t ev, process_data_t data)
{
#if UIP_TCP
  unsigned char i;
  register struct listenport *l;
#endif /*UIP_TCP*/
  struct process *p;

  switch(ev) {
  case PROCESS_EVENT_EXITED:
    /* This is the event we get if a process has exited. We go through
         the TCP/IP tables to see if this process had any open
         connections or listening TCP ports. If so, we'll close those
         connections. */

    p = (struct process *)data;
#if UIP_TCP
    l = s.listenports;
    for(i = 0; i < UIP_LISTENPORTS; ++i) {
      if(l->p == p) {
        uip_unlisten(l->port);
        l->port = 0;
        l->p = PROCESS_NONE;
      }
      ++l;
    }

    {
      struct uip_conn *cptr;

      for(cptr = &uip_conns[0]; cptr < &uip_conns[UIP_CONNS]; ++cptr) {
        if(cptr->appstate.p == p) {
          cptr->appstate.p = PROCESS_NONE;
          cptr->tcpstateflags = UIP_CLOSED;
        }
      }
    }
#endif /* UIP_TCP */
#if UIP_UDP
    {
      struct uip_udp_conn *cptr;

      for(cptr = &uip_udp_conns[0];
          cptr < &uip_udp_conns[UIP_UDP_CONNS]; ++cptr) {
        if(cptr->appstate.p == p) {
          cptr->lport = 0;
        }
      }
    }
#endif /* UIP_UDP */
    break;

  case PROCESS_EVENT_TIMER:
    /* We get this event if one of our timers have expired. */
  {
    /* Check the clock so see if we should call the periodic uIP
           processing. */
    if(data == &periodic &&
        etimer_expired(&periodic)) {
#if UIP_TCP
      for(i = 0; i < UIP_CONNS; ++i) {
        if(uip_conn_active(i)) {
          /* Only restart the timer if there are active
                 connections. */
          etimer_restart(&periodic);
          uip_periodic(i);
#if NETSTACK_CONF_WITH_IPV6
          tcpip_ipv6_output();
#else
          if(uip_len > 0) {
            PRINTF("tcpip_output from periodic len %d\n", uip_len);
            tcpip_output();
            PRINTF("tcpip_output after periodic len %d\n", uip_len);
          }
#endif /* NETSTACK_CONF_WITH_IPV6 */
        }
      }
#endif /* UIP_TCP */
#if UIP_CONF_IP_FORWARD
      uip_fw_periodic();
#endif /* UIP_CONF_IP_FORWARD */
    }

#if NETSTACK_CONF_WITH_IPV6
#if UIP_CONF_IPV6_REASSEMBLY
    /*
     * check the timer for reassembly
     */
    if(data == &uip_reass_timer &&
        etimer_expired(&uip_reass_timer)) {
      uip_reass_over();
      tcpip_ipv6_output();
    }
#endif /* UIP_CONF_IPV6_REASSEMBLY */
    /*
     * check the different timers for neighbor discovery and
     * stateless autoconfiguration
     */
    /*if(data == &uip_ds6_timer_periodic &&
           etimer_expired(&uip_ds6_timer_periodic)) {
          uip_ds6_periodic();
          tcpip_ipv6_output();
        }*/
#if !UIP_CONF_ROUTER
    if(data == &uip_ds6_timer_rs &&
        etimer_expired(&uip_ds6_timer_rs)) {
      uip_ds6_send_rs();
      tcpip_ipv6_output();
    }
#endif /* !UIP_CONF_ROUTER */
    if(data == &uip_ds6_timer_periodic &&
        etimer_expired(&uip_ds6_timer_periodic)) {
      uip_ds6_periodic();
      tcpip_ipv6_output();
    }
#endif /* NETSTACK_CONF_WITH_IPV6 */
  }
  break;

#if UIP_TCP
  case TCP_POLL:
    if(data != NULL) {
      uip_poll_conn(data);
#if NETSTACK_CONF_WITH_IPV6
      tcpip_ipv6_output();
#else /* NETSTACK_CONF_WITH_IPV6 */
      if(uip_len > 0) {
        PRINTF("tcpip_output from tcp poll len %d\n", uip_len);
        tcpip_output();
      }
#endif /* NETSTACK_CONF_WITH_IPV6 */
      /* Start the periodic polling, if it isn't already active. */
      start_periodic_tcp_timer();
    }
    break;
#endif /* UIP_TCP */
#if UIP_UDP
  case UDP_POLL:
    if(data != NULL) {
      uip_udp_periodic_conn(data);
#if NETSTACK_CONF_WITH_IPV6
      tcpip_ipv6_output();
#else
      if(uip_len > 0) {
        tcpip_output();
      }
#endif /* UIP_UDP */
    }
    break;
#endif /* UIP_UDP */

  case PACKET_INPUT:
    packet_input();
    break;
  };
}
/*---------------------------------------------------------------------------*/
void
tcpip_input(void)
{
  process_post_synch(&tcpip_process, PACKET_INPUT, NULL);
  uip_clear_buf();
}
/*---------------------------------------------------------------------------*/
#if NETSTACK_CONF_WITH_IPV6
void
tcpip_ipv6_output(void)
{
  uip_ds6_nbr_t *nbr = NULL;
  uip_ipaddr_t *nexthop = NULL;

  if(uip_len == 0) {
    return;
  }

  if(uip_len > UIP_LINK_MTU) {
    UIP_LOG("tcpip_ipv6_output: Packet to big");
    uip_clear_buf();
    return;
  }

  if(uip_is_addr_unspecified(&UIP_IP_BUF->destipaddr)){
    UIP_LOG("tcpip_ipv6_output: Destination address unspecified");
    uip_clear_buf();
    return;
  }

#if UIP_CONF_IPV6_RPL
  if(!rpl_update_header()) {
    /* Packet can not be forwarded */
    PRINTF("tcpip_ipv6_output: RPL header update error\n");
    uip_clear_buf();
    return;
  }
#endif /* UIP_CONF_IPV6_RPL */

  if(!uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    /* Next hop determination */

#if UIP_CONF_IPV6_RPL && RPL_WITH_NON_STORING
    uip_ipaddr_t ipaddr;
    /* Look for a RPL Source Route */
    if(rpl_srh_get_next_hop(&ipaddr)) {
      nexthop = &ipaddr;
    }
#endif /* UIP_CONF_IPV6_RPL && RPL_WITH_NON_STORING */

    nbr = NULL;

    /* We first check if the destination address is on our immediate
       link. If so, we simply use the destination address as our
       nexthop address. */
    if(nexthop == NULL && uip_ds6_is_addr_onlink(&UIP_IP_BUF->destipaddr)){
      nexthop = &UIP_IP_BUF->destipaddr;
    }

    if(nexthop == NULL) {
      uip_ds6_route_t *route;
      /* Check if we have a route to the destination address. */
      route = uip_ds6_route_lookup(&UIP_IP_BUF->destipaddr);

      /* No route was found - we send to the default route instead. */
      if(route == NULL) {
        PRINTF("tcpip_ipv6_output: no route found, using default route\n");
        nexthop = uip_ds6_defrt_choose();
        if(nexthop == NULL) {
#ifdef UIP_FALLBACK_INTERFACE
          PRINTF("FALLBACK: removing ext hdrs & setting proto %d %d\n",
              uip_ext_len, *((uint8_t *)UIP_IP_BUF + 40));
          if(uip_ext_len > 0) {
            extern void remove_ext_hdr(void);
            uint8_t proto = *((uint8_t *)UIP_IP_BUF + 40);
            remove_ext_hdr();
            /* This should be copied from the ext header... */
            UIP_IP_BUF->proto = proto;
          }
          /* Inform the other end that the destination is not reachable. If it's
           * not informed routes might get lost unexpectedly until there's a need
           * to send a new packet to the peer */
          if(UIP_FALLBACK_INTERFACE.output() < 0) {
            PRINTF("FALLBACK: output error. Reporting DST UNREACH\n");
            uip_icmp6_error_output(ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_ADDR, 0);
            uip_flags = 0;
            tcpip_ipv6_output();
            return;
          }
#else
          PRINTF("tcpip_ipv6_output: Destination off-link but no route\n");
#endif /* !UIP_FALLBACK_INTERFACE */
          uip_clear_buf();
          return;
        }

      } else {
        /* A route was found, so we look up the nexthop neighbor for
           the route. */
        nexthop = uip_ds6_route_nexthop(route);

        /* If the nexthop is dead, for example because the neighbor
           never responded to link-layer acks, we drop its route. */
        if(nexthop == NULL) {
#if UIP_CONF_IPV6_RPL
          /* If we are running RPL, and if we are the root of the
             network, we'll trigger a global repair berfore we remove
             the route. */
          rpl_dag_t *dag;
          rpl_instance_t *instance;

          dag = (rpl_dag_t *)route->state.dag;
          if(dag != NULL) {
            instance = dag->instance;

            rpl_repair_root(instance->instance_id);
          }
#endif /* UIP_CONF_IPV6_RPL */
          uip_ds6_route_rm(route);

          /* We don't have a nexthop to send the packet to, so we drop
             it. */
          return;
        }
      }
#if TCPIP_CONF_ANNOTATE_TRANSMISSIONS
      if(nexthop != NULL) {
        static uint8_t annotate_last;
        static uint8_t annotate_has_last = 0;

        if(annotate_has_last) {
          printf("#L %u 0; red\n", annotate_last);
        }
        printf("#L %u 1; red\n", nexthop->u8[sizeof(uip_ipaddr_t) - 1]);
        annotate_last = nexthop->u8[sizeof(uip_ipaddr_t) - 1];
        annotate_has_last = 1;
      }
#endif /* TCPIP_CONF_ANNOTATE_TRANSMISSIONS */
    }

    /* End of next hop determination */

    nbr = uip_ds6_nbr_lookup(nexthop);
    if(nbr == NULL) {
#if UIP_ND6_SEND_NS
      if((nbr = uip_ds6_nbr_add(nexthop, NULL, 0, NBR_INCOMPLETE, NBR_TABLE_REASON_IPV6_ND, NULL)) == NULL) {
        uip_clear_buf();
        PRINTF("tcpip_ipv6_output: failed to add neighbor to cache\n");
        return;
      } else {
#if UIP_CONF_IPV6_QUEUE_PKT
        /* Copy outgoing pkt in the queuing buffer for later transmit. */
        if(uip_packetqueue_alloc(&nbr->packethandle, UIP_DS6_NBR_PACKET_LIFETIME) != NULL) {
          memcpy(uip_packetqueue_buf(&nbr->packethandle), UIP_IP_BUF, uip_len);
          uip_packetqueue_set_buflen(&nbr->packethandle, uip_len);
        }
#endif
        /* RFC4861, 7.2.2:
         * "If the source address of the packet prompting the solicitation is the
         * same as one of the addresses assigned to the outgoing interface, that
         * address SHOULD be placed in the IP Source Address of the outgoing
         * solicitation.  Otherwise, any one of the addresses assigned to the
         * interface should be used."*/
        if(uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
          uip_nd6_ns_output(&UIP_IP_BUF->srcipaddr, NULL, &nbr->ipaddr);
        } else {
          uip_nd6_ns_output(NULL, NULL, &nbr->ipaddr);
        }

        stimer_set(&nbr->sendns, uip_ds6_if.retrans_timer / 1000);
        nbr->nscount = 1;
        /* Send the first NS try from here (multicast destination IP address). */
      }
#else /* UIP_ND6_SEND_NS */
      PRINTF("tcpip_ipv6_output: neighbor not in cache\n");
      uip_len = 0;
      return;  
#endif /* UIP_ND6_SEND_NS */
    } else {
#if UIP_ND6_SEND_NS
      if(nbr->state == NBR_INCOMPLETE) {
        PRINTF("tcpip_ipv6_output: nbr cache entry incomplete\n");
#if UIP_CONF_IPV6_QUEUE_PKT
        /* Copy outgoing pkt in the queuing buffer for later transmit and set
           the destination nbr to nbr. */
        if(uip_packetqueue_alloc(&nbr->packethandle, UIP_DS6_NBR_PACKET_LIFETIME) != NULL) {
          memcpy(uip_packetqueue_buf(&nbr->packethandle), UIP_IP_BUF, uip_len);
          uip_packetqueue_set_buflen(&nbr->packethandle, uip_len);
        }
#endif /*UIP_CONF_IPV6_QUEUE_PKT*/
        uip_clear_buf();
        return;
      }
      /* Send in parallel if we are running NUD (nbc state is either STALE,
         DELAY, or PROBE). See RFC 4861, section 7.3.3 on node behavior. */
      if(nbr->state == NBR_STALE) {
        nbr->state = NBR_DELAY;
        stimer_set(&nbr->reachable, UIP_ND6_DELAY_FIRST_PROBE_TIME);
        nbr->nscount = 0;
        PRINTF("tcpip_ipv6_output: nbr cache entry stale moving to delay\n");
      }
#endif /* UIP_ND6_SEND_NS */

      tcpip_output(uip_ds6_nbr_get_ll(nbr));

#if UIP_CONF_IPV6_QUEUE_PKT
      /*
       * Send the queued packets from here, may not be 100% perfect though.
       * This happens in a few cases, for example when instead of receiving a
       * NA after sendiong a NS, you receive a NS with SLLAO: the entry moves
       * to STALE, and you must both send a NA and the queued packet.
       */
      if(uip_packetqueue_buflen(&nbr->packethandle) != 0) {
        uip_len = uip_packetqueue_buflen(&nbr->packethandle);
        memcpy(UIP_IP_BUF, uip_packetqueue_buf(&nbr->packethandle), uip_len);
        uip_packetqueue_free(&nbr->packethandle);
        tcpip_output(uip_ds6_nbr_get_ll(nbr));
      }
#endif /*UIP_CONF_IPV6_QUEUE_PKT*/

      uip_clear_buf();
      return;
    }
  }
  /* Multicast IP destination address. */
  tcpip_output(NULL);
  uip_clear_buf();
}
#endif /* NETSTACK_CONF_WITH_IPV6 */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
void
tcpip_poll_udp(struct uip_udp_conn *conn)
{
  process_post(&tcpip_process, UDP_POLL, conn);
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_TCP
void
tcpip_poll_tcp(struct uip_conn *conn)
{
  process_post(&tcpip_process, TCP_POLL, conn);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
void
tcpip_uipcall(void)
{
  uip_udp_appstate_t *ts;

#if UIP_UDP
  if(uip_conn != NULL) {
    ts = &uip_conn->appstate;
  } else {
    ts = &uip_udp_conn->appstate;
  }
#else /* UIP_UDP */
  ts = &uip_conn->appstate;
#endif /* UIP_UDP */

#if UIP_TCP
  {
    unsigned char i;
    struct listenport *l;

    /* If this is a connection request for a listening port, we must
      mark the connection with the right process ID. */
    if(uip_connected()) {
      l = &s.listenports[0];
      for(i = 0; i < UIP_LISTENPORTS; ++i) {
        if(l->port == uip_conn->lport &&
            l->p != PROCESS_NONE) {
          ts->p = l->p;
          ts->state = NULL;
          break;
        }
        ++l;
      }

      /* Start the periodic polling, if it isn't already active. */
      start_periodic_tcp_timer();
    }
  }
#endif /* UIP_TCP */

  if(ts->p != NULL) {
    process_post_synch(ts->p, tcpip_event, ts->state);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcpip_process, ev, data)
{
  PROCESS_BEGIN();

#if UIP_TCP
  {
    unsigned char i;

    for(i = 0; i < UIP_LISTENPORTS; ++i) {
      s.listenports[i].port = 0;
    }
    s.p = PROCESS_CURRENT();
  }
#endif

  tcpip_event = process_alloc_event();
#if UIP_CONF_ICMP6
  tcpip_icmp6_event = process_alloc_event();
#endif /* UIP_CONF_ICMP6 */
  etimer_set(&periodic, CLOCK_SECOND / 2);

  uip_init();
#ifdef UIP_FALLBACK_INTERFACE
  UIP_FALLBACK_INTERFACE.init();
#endif
  /* initialize RPL if configured for using RPL */
#if NETSTACK_CONF_WITH_IPV6 && UIP_CONF_IPV6_RPL
  rpl_init();
#endif /* UIP_CONF_IPV6_RPL */

  while(1) {
    PROCESS_YIELD();
    eventhandler(ev, data);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

#if NETWORK_CODING 
/* extract coap payloads and keep them until its time, this message is when ncoding file decides*/
void 
store_msg(void){
  //static target_etag=""
  /* This is a definition put in Contiki/platform/wismote/platform-conf.c. Sky motes do not have enough memory to implement Aggregation. */
  //is my packet uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)
  static coap_packet_t coap_pt[1];    //allocate space for 1 packet, treat as pointer
  static unsigned int begin_payload_index=UIP_IPUDPH_LEN+8;  // the forwarded packet has 8 more bytes(extension headers=uip_ext_len)
  /*IMPORTANT, because of max packet size in REST_MAX_CHUNK_SIZE tha max size of parsed packet is that value*/
  /*This function is used in coap receive, the size given (arg3) has to be the size of ONLY the coap packet*/
  coap_parse_message(coap_pt, &uip_buf[begin_payload_index], uip_datalen()-begin_payload_index);
  if(coap_pt->code==69 && coap_pt->version ==1){     
    /*todo, messages from coded have etal of 0x0a, verify here if it is ok*/
    if(IS_OPTION(coap_pt, COAP_OPTION_ETAG)) {
      //printf("storing message, etag= %u\n",coap_pt->etag);
      /*check correct etag from coded resource, is 10*/
      if (((int)coap_pt->etag[0]==10))//etag will sufice, the ports change so do not use that, also they apear on the network order(reverse)
      {
        add_payload(coap_pt->payload, coap_pt->mid, (uint8_t)coap_pt->payload_len, &UIP_IP_BUF->destipaddr,UIP_UDP_BUF->destport, coap_pt);
      }
    }
      /* Drop all not self-produced packets., NOT IN THIS CASE WITH CODING
      uip_len = 0;
      uip_ext_len = 0;
      uip_flags = 0;
      return;*/
    } 
}
#endif

#if GILBERT_ELLIOT_DISCARDER
/*
*Returns 1 if discarded packet
*/
int discard_engine(int _node_id){

// gilbert-elliot
// two-state markov chain
static int discardPkt = 0;
static int goodState = 1;

// todos os valores vao de 0% a 100%
// quantidade de perdas no estado Good
static int LostInGood = 0;
// quantidade de perdas no estado Bad
static int LostInBad = 100;
// proporcao de troca de estado - no project conf
//static int GoodToBad = 20;
//static int BadToGood = 60;
if (goodState) {

    // verificar se deve perder o pacote    
    if ( (1+random_rand()%100) <= LostInGood ){
        discardPkt = 1;
    }else {
        discardPkt = 0;
    }
    // verifica se deve mudar de estado
    if ( (1+random_rand()%100) <= GoodToBad ) {
        goodState = 0;
        
    } else {
        goodState = 1;
    }
       // cout de debug
    //cout << "GoodState" << (discardPkt?"D":".") << (goodState?"+":"-") << endl;
    PRINTF("Goodstate: discard=%d goodstate=%d\n",discardPkt,goodState );
} else {

    // verificar se deve perder o pacote
    if ( (1+random_rand()%100) <= LostInBad ){
        discardPkt = 1;
    }else {
        discardPkt = 0;
    }
    // verifica se deve mudar de estado
    if ( (1+random_rand()%100) <= BadToGood ) {
        goodState = 1;
    } else {
        goodState = 0;
        
    }
       // cout de debug 
    //cout << "BadState" << (discardPkt?"D":".") << (goodState?"+":"-") << endl;
    PRINTF("Badstate: discard=%d goodstate=%d\n",discardPkt,goodState );
}
// faz o descarte do pacote se necessario
  if (discardPkt) {
      //printf("Discarding packet from _node_id=%d\n",_node_id);
      uip_len = 0;
      uip_ext_len = 0;
      uip_flags = 0;
      total_dropped+=1;
      return 1;
  }
  total_forwarded++;
  print_mid(_node_id);
  return 0;

}



void print_mid(int id){
  static coap_packet_t coap_pt[1];    //allocate space for 1 packet, treat as pointer
  static unsigned int begin_payload_index=UIP_IPUDPH_LEN+8;  // the forwarded packet has 8 more bytes(extension headers=uip_ext_len)
  /*IMPORTANT, because of max packet size in REST_MAX_CHUNK_SIZE tha max size of parsed packet is that value*/
  /*This function is used in coap receive, the size given (arg3) has to be the size of ONLY the coap packet*/
  coap_parse_message(coap_pt, &uip_buf[begin_payload_index], uip_datalen()-begin_payload_index);
  if(coap_pt->code==69 && coap_pt->version ==1){      
      PRINTF("Node-id=%d, mid= %u\n",id,coap_pt->mid);
      
    } 
}
#endif
