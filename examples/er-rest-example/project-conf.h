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

#define COOJA_EXP 1  //use this to apply configurations for development in cooja

#define NETWORK_CODING 1   /*activates packet capturing and network coding mechanism */
#define MAX_CODED_PAYLOADS 4  /* max number of packets in the buffer to code*/
#define TRIGGERPACKETS 4 /*number of packets that trigger a coded message*/
#define OBS_REFRESH_INTERVAL 20 /*interval of ack in messages, 1 to allways confirmable*/
#define SEND_MESSAGE_INTERVAL 10 /*interval to send observable, coded resource messages*/
#define MAX_RETRANS 4 /*max number of retransmissions of a single message*/

/*variables to gather experiments data*/
extern unsigned int count_retrans;
extern unsigned int count_ack;
extern unsigned int total_coap_sent; /*number of coap messages sent including retransmissions*/

#if NETWORK_CODING
#define RPL_UPDATE_INTERVAL 600 //testing in net/rpl/conf
#define HARDCODED_TOPOLOGY 1
#define PARENT_IP "fe80::200:0:0:2" 
//undef para ativar
#undef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#undef RF2XX_TX_POWER
#define RF2XX_TX_POWER  0x0F  //PHY_POWER_m12dBm, before lowest level of power(0,1,2,3,4(0x0A),5(B),7(C),9(D),12(E),17(F))
#endif


/*for doing tests in coojs and debugging*/
# if COOJA_EXP

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

/*
#undef RF2XX_CHANNEL
#define RF2XX_CHANNEL 5


#undef RF2XX_TX_POWER
#define RF2XX_TX_POWER  0x0F  //PHY_POWER_m17dBm

#undef RF2XX_RX_RSSI_THRESHOLD
#define RF2XX_RX_RSSI_THRESHOLD  0xb //RF2XX_PHY_RX_THRESHOLD__m60dBm
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
#define COAP_MAX_OPEN_TRANSACTIONS     4

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
#define COAP_OBSERVE_CLIENT 1
#endif /* __PROJECT_ERBIUM_CONF_H__ */
