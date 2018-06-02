/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *      Erbium (Er) example project configuration.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef __PROJECT_ERBIUM_CONF_H__
#define __PROJECT_ERBIUM_CONF_H__

/*densenet: network coding mechanism*/
/*add files for packet capturing and network coding functions*/
/*variables to gather experiments data*/
extern  int count_retrans;
extern  int count_ack;
extern  int total_coap_sent; /*number of coap messages sent including retransmissions*/
/*variables for the gilbert elliot discard statistics*/
extern  int total_forwarded;
extern  int total_dropped;
extern  int from_node3;
extern  int from_node4;
extern  int lostpackets;

#define ELEMENTS 13
extern  int loss_array[ELEMENTS];

/*fag forlafgts reset and discarder, if its one discarding begins */
extern  int dis_flag;


#define SEND_MESSAGE_INTERVAL 1    /*interval to send coded resource messages*/
#define WARMUP_DISCARD 190          /*DISCARD FIRST X SECONDS OF STATISTICS*/
#define MAX_CODED_PAYLOADS 30        /* space in number of packets in the buffer to code*/
#define TRIGGERPACKETS 2        /*number of packets that trigger a coded message*/
#define OBS_REFRESH_INTERVAL 700      /*interval of ack in messages, 1 to allways confirmable*/
#define MAX_RETRANS 4               /*max number of retransmissions of a single message*/
#define COM_ACKS 1 /*TO ADD HEADER AND FUNCTION DECLARATIONS IN TCPIP*/
/*gilber elliot transition probabilities from 0 to 100*/
#define GoodToBad 11
#define BadToGood 45
/*
n1-   3/84
n2-   25/42
n3-   43/25
n4-   80/7
*/
/*logic, use discarded and coding + iotlab for iotlab, when testing in cooja change flags cooja exp */
#define NETWORK_CODING 0            /*activates packet capturing and network coding mechanism */
#define COOJA_EXP 0     /*use this to apply configurations for development in cooja*/
#define IOTLAB 1        /*reduces power and other stuff for testing in iotlab*/
#define HARDCODED_TOPOLOGY 1 /*static routing*/
#define GILBERT_ELLIOT_DISCARDER 1    /*this macro sends packets to loss model*/
//#define REQUEST_NODE(ipaddr)   uip_ip6addr(ipaddr, 0x2001, 0x0660, 0x5307, 0x3111, 0, 0, 0 , 0x0001) 

#if GILBERT_ELLIOT_DISCARDER
extern  int local_loss; /*the coded messages can also be lost, count the lost coded messages*/
extern  int sent_coded;
#endif
#if IOTLAB
//#define RPL_UPDATE_INTERVAL 600 //testing in net/rpl/conf
#if HARDCODED_TOPOLOGY
#define PARENT_IP "fe80::2661"
#define NODE_7_IP 0x2453
#define NODE_8_IP 0x2061
#define NODE_9_IP 0x2460
#define NODE_10_IP 0x4061
#define NODE_11_IP 0xc471
#define NODE_12_IP 0x3560
#endif
//#undef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
//#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#undef RF2XX_TX_POWER
#define RF2XX_TX_POWER  0x0F  //PHY_POWER_m12dBm, before lowest level of power(0,1,2,3,4(0x0A),5(B),7(C),9(D),12(E),17(F))
#endif

/*for doing tests in coojs and debugging*/
# if COOJA_EXP
#define NODE_3_IP 3
#define NODE_4_IP 4

//#warning This is being compiled for a cooja experiment scenario
#define PERIODIC_MESSAGE 0 /*activate periodic resource for auto send coded */
extern int id_node; /*for cooja tests, used in tcpip*/
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC              nullrdc_driver 
#undef RPL_CONF_MAX_DAG_PER_INSTANCE
#define RPL_CONF_MAX_DAG_PER_INSTANCE     1
/* Disabling TCP on CoAP nodes. */
#undef UIP_CONF_TCP
#define UIP_CONF_TCP                   0
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     nullmac_driver

#endif

/*to adjust for multihop
* to avoid including header files for power check the page below for the hex
* https://github.com/iot-lab/openlab/blob/master/periph/rf2xx/rf2xx_regs.h#L236

   RF2XX_PHY_TX_PWR_TX_PWR_VALUE__3dBm = 0x00,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__2_8dBm = 0x01,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__2_3dBm = 0x02,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__1_8dBm = 0x03,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__1_3dBm = 0x04,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__0_7dBm = 0x05,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__0dBm = 0x06,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m1dBm = 0x07,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m2dBm = 0x08,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m3dBm = 0x09,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m4dBm = 0x0A,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m5dBm = 0x0B,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m7dBm = 0x0C,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m9dBm = 0x0D,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m12dBm = 0x0E,
    RF2XX_PHY_TX_PWR_TX_PWR_VALUE__m17dBm = 0x0F,
*/

/* Custom channel and PAN ID configuration for your project. */
/*
   #undef RF_CHANNEL
   #define RF_CHANNEL                     26

   #undef IEEE802154_CONF_PANID
   #define IEEE802154_CONF_PANID          0xABCD
 */

/* IP buffer size must match all other hops, in particular the border router. */
/*
   #undef UIP_CONF_BUFFER_SIZE
   #define UIP_CONF_BUFFER_SIZE           256
 */

/* Disabling RDC and CSMA for demo purposes. Core updates often
   require more memory. */
/* For projects, optimize memory and enable RDC and CSMA again. */
/*
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC              nullrdc_driver

#undef RPL_CONF_MAX_DAG_PER_INSTANCE
#define RPL_CONF_MAX_DAG_PER_INSTANCE     1

 Disabling TCP on CoAP nodes. */


/* Increase rpl-border-router IP-buffer when using more than 64. */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE            64 /*densenet: antes era 48*/

/* Estimate your header size, especially when using Proxy-Uri. */
/*
   #undef COAP_MAX_HEADER_SIZE
   #define COAP_MAX_HEADER_SIZE           70
 */

/* Multiplies with chunk size, be aware of memory constraints. */
#undef COAP_MAX_OPEN_TRANSACTIONS
#define COAP_MAX_OPEN_TRANSACTIONS     10

/* Must be <= open transactions, default is COAP_MAX_OPEN_TRANSACTIONS-1. */
/*
   #undef COAP_MAX_OBSERVERS
   #define COAP_MAX_OBSERVERS             2
 */

/* Filtering .well-known/core per query can be disabled to save space. */
#undef COAP_LINK_FORMAT_FILTERING
#define COAP_LINK_FORMAT_FILTERING     0
#undef COAP_PROXY_OPTION_PROCESSING
#define COAP_PROXY_OPTION_PROCESSING   0

/* Turn off DAO ACK to make code smaller */
#undef RPL_CONF_WITH_DAO_ACK
#define RPL_CONF_WITH_DAO_ACK          0

/* Enable client-side support for COAP observe */
#define COAP_OBSERVE_CLIENT 0
#endif /* __PROJECT_ERBIUM_CONF_H__ */
