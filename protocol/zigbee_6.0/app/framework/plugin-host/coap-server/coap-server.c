/* coap -- simple implementation of the Constrained Application Protocol (CoAP)
 *         as defined in RFC 7252
 *
 * Copyright (C) 2010--2016 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

#include PLATFORM_HEADER
#ifdef EZSP_HOST
// Includes needed for functions related to the EZSP host
  #include "stack/include/error.h"
  #include "stack/include/ember-types.h"
  #include "app/util/ezsp/ezsp-protocol.h"
  #include "app/util/ezsp/ezsp.h"
  #include "app/util/ezsp/serial-interface.h"
  #include "app/util/zigbee-framework/zigbee-device-common.h"
#else
  #include "stack/include/ember.h"
#endif

#include "af.h"
#include "af-main.h"
#include "hal/hal.h"
#include "app/util/serial/command-interpreter2.h"
#include "stack/include/event.h"
#include "app/framework/plugin/concentrator/source-route-host.h"
#include "app/framework/plugin/device-table/device-table.h"
#include "app/framework/plugin/device-table/device-table-internal.h"
#include <stdlib.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#include "coap_config.h"
#include "include/coap/resource.h"
#include "include/coap/coap.h"

#include "app/framework/plugin-host/gateway-relay-coap/gateway-relay-coap.h"
#include "app/framework/plugin-host/coap-server/coap-server.h"

#define COAP_RESOURCE_CHECK_TIME 2

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// making main variables globals
#define MAX_PORTS PLUGIN_COAP_SERVER_MAX_PORTS
#define BASE_DEVICE_PORT 5700
static coap_context_t  *ctx = NULL;
static coap_context_t  *ctxArray[MAX_PORTS] = { NULL, };
static fd_set readfds;
static struct timeval tv, *timeout;
static int result;
static coap_tick_t now;
static coap_queue_t *nextpdu;
static char addr_str[NI_MAXHOST] = "::";
static char port_str[NI_MAXSERV] = "5683";
static char devicePortString[NI_MAXSERV] = "5700";
static coap_log_t log_level = LOG_WARNING;

#define MAX_STRING 50
#define ZCL_OPTION_LENGTH 3

// current port being read.
static int currentPort = -1;
#define COAP_HEADER_LENGTH 9

/* changeable clock base (see handle_put_time()) */
static time_t clock_offset;

struct coap_resource_t *time_resource = NULL;

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

static void possiblySetServerAddress(uint8_t *addr);

// ******** ZCL handling *******
static void printData(uint8_t * data, uint16_t length, char *formatString)
{
  uint16_t i;
  printf("Data: ");

  for (i = 0; i < length; i++) {
    printf(formatString, data[i]);
  }

  printf("\r\n");
}

static void zclProcess(coap_context_t  *ctx,
                       struct coap_resource_t *resource,
                       const coap_endpoint_t *local_interface UNUSED_PARAM,
                       coap_address_t *peer,
                       coap_pdu_t *request,
                       str *token,
                       coap_pdu_t *response,
                       uint8_t method)
{
  uint16_t dotdotLength = request->length - COAP_HEADER_LENGTH;
  uint8_t *dotdotMessage = (uint8_t *) malloc(dotdotLength + 1);

  uint8_t *responseString;
  uint16_t responseLength;

  printf("Incoming ADDR:  ");
  printData(peer->addr.sin6.sin6_addr.s6_addr, 16, "%d ");

  // Set the outgoing message destination if not already set.
  possiblySetServerAddress(peer->addr.sin6.sin6_addr.s6_addr);

  // Copy data into the buffer for parsing.
  MEMCOPY(dotdotMessage, ((uint8_t *) request->hdr) + COAP_HEADER_LENGTH, dotdotLength);

  emberAfGatewayRelayCoapParseDotdotMessage(dotdotLength - ZCL_OPTION_LENGTH,
                                            dotdotMessage + ZCL_OPTION_LENGTH,
                                            currentPort,
                                            method);

  free(dotdotMessage);

  responseLength = emberAfPluginGatewayRelayCoapReturnStringLength();

  if (responseLength > 0) {
    responseString = (uint8_t *) malloc(responseLength);

    emberAfPluginGatewayRelayCoapCopyReturnString((char *) responseString);

    response->hdr->code = COAP_RESPONSE_CODE(205);

    unsigned char buf[3];

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE,
                    coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response,
                    COAP_OPTION_MAXAGE,
                    coap_encode_var_bytes(buf, 0x2ffff), buf);

    coap_add_data(response, responseLength, responseString);
    free(responseString);
  } else {
    unsigned char buf[3];

    response->hdr->code = COAP_RESPONSE_CODE(emberAfPluginGatewayRelayCoapReturnCode());
    coap_add_option(response, COAP_OPTION_CONTENT_TYPE,
                    coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response,
                    COAP_OPTION_MAXAGE,
                    coap_encode_var_bytes(buf, 0x2ffff), buf);
  }
}

static void zclGet(coap_context_t  *ctx,
                   struct coap_resource_t *resource,
                   const coap_endpoint_t *local_interface UNUSED_PARAM,
                   coap_address_t *peer,
                   coap_pdu_t *request,
                   str *token,
                   coap_pdu_t *response)
{
  zclProcess(ctx, resource, local_interface, peer, request, token, response, COAP_REQUEST_GET);
}

static void zclPut(coap_context_t  *ctx,
                   struct coap_resource_t *resource,
                   const coap_endpoint_t *local_interface UNUSED_PARAM,
                   coap_address_t *peer,
                   coap_pdu_t *request,
                   str *token,
                   coap_pdu_t *response)
{
  zclProcess(ctx, resource, local_interface, peer, request, token, response, COAP_REQUEST_PUT);
}

static void zclPost(coap_context_t  *ctx,
                    struct coap_resource_t *resource,
                    const coap_endpoint_t *local_interface UNUSED_PARAM,
                    coap_address_t *peer,
                    coap_pdu_t *request,
                    str *token,
                    coap_pdu_t *response)
{
  zclProcess(ctx, resource, local_interface, peer, request, token, response, COAP_REQUEST_POST);
}

static void zclDelete(coap_context_t  *ctx,
                      struct coap_resource_t *resource,
                      const coap_endpoint_t *local_interface UNUSED_PARAM,
                      coap_address_t *peer,
                      coap_pdu_t *request,
                      str *token,
                      coap_pdu_t *response)
{
  zclProcess(ctx, resource, local_interface, peer, request, token, response, COAP_REQUEST_DELETE);
}

static void rdGet(coap_context_t  *ctx,
                  struct coap_resource_t *resource,
                  const coap_endpoint_t *local_interface UNUSED_PARAM,
                  coap_address_t *peer,
                  coap_pdu_t *request,
                  str *token,
                  coap_pdu_t *response)
{
  uint8_t *responseString = (uint8_t *) malloc(1500); // zeroed space.
  uint8_t *rsPointer;
  uint16_t responseLength;

  uint16_t i;

  coap_address_t *localAddress = &(((coap_endpoint_t *)local_interface)->addr);

  // First, let's try to print the local address.
  for (i = 0; i < 16; i++) {
    printf("%d ", localAddress->addr.sin6.sin6_addr.s6_addr[i]);
  }
  printf("%d\r\n", localAddress->addr.sin6.sin6_port);

  rsPointer = responseString;

  printf("RD GET\r\n");
  unsigned char buf[3];

  response->hdr->code = COAP_RESPONSE_CODE(205);

  coap_add_option(response, COAP_OPTION_CONTENT_TYPE,
                  coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  coap_encode_var_bytes(buf, 0x2ffff), buf);

  for (i = 0; i < MAX_PORTS; i++) {
    if (ctxArray[i] != NULL) {
      responseLength = sprintf((char *) rsPointer,
                               "{%d: {\"rd\": coap://%d.%d.%d.%d:%d/zcl;rt=urn:zcl;if=urn:zcl:c.v1;ze=urn:zcl:d.101.1}",
                               i + 1,
                               // assuming ipv4 for now.
                               localAddress->addr.sin6.sin6_addr.s6_addr[12],
                               localAddress->addr.sin6.sin6_addr.s6_addr[13],
                               localAddress->addr.sin6.sin6_addr.s6_addr[14],
                               localAddress->addr.sin6.sin6_addr.s6_addr[15],
                               BASE_DEVICE_PORT + i);

      rsPointer += responseLength;
    }
  }
  responseLength = strlen((char *)responseString);

  coap_add_data(response, responseLength, responseString);
  free(responseString);
}

static void wellKnownGet(coap_context_t  *ctx,
                         struct coap_resource_t *resource,
                         const coap_endpoint_t *local_interface UNUSED_PARAM,
                         coap_address_t *peer,
                         coap_pdu_t *request,
                         str *token,
                         coap_pdu_t *response)
{
  // all Zigbee devices have a common well-known/core response
  uint8_t *responseString = (uint8_t *)"</zcl>;rt=urn:zcl;if=urn:zcl:v0";
  uint16_t responseLength = 31;

  printf("Well-known\r\n");

  if (responseLength > 0) {
    response->hdr->code = COAP_RESPONSE_CODE(205);

    unsigned char buf[3];

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE,
                    coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response,
                    COAP_OPTION_MAXAGE,
                    coap_encode_var_bytes(buf, 0x2ffff), buf);

    coap_add_data(response, responseLength, responseString);
  }
}

static void init_resources(coap_context_t *ctx)
{
  coap_resource_t *r;

  r = coap_resource_init((unsigned char *)"zcl", 3, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, zclGet);
  coap_register_handler(r, COAP_REQUEST_PUT, zclPut);
  coap_register_handler(r, COAP_REQUEST_POST, zclPost);
  coap_register_handler(r, COAP_REQUEST_DELETE, zclDelete);
  coap_add_resource(ctx, r);

  r = coap_resource_init((unsigned char *)"rd", 2, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, rdGet);
  coap_add_resource(ctx, r);

  r = coap_resource_init((unsigned char *)".well-known", 11, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, wellKnownGet);
  coap_add_resource(ctx, r);
}

static coap_context_t *get_context(const char *node, const char *port)
{
  coap_context_t *ctx = NULL;
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

  s = getaddrinfo(node, port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return NULL;
  }

  /* iterate through results until success */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    coap_address_t addr;

    if (rp->ai_addrlen <= sizeof(addr.addr)) {
      coap_address_init(&addr);
      addr.size = rp->ai_addrlen;
      memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);

      ctx = coap_new_context(&addr);
      if (ctx) {
        goto finish;
      }
    }
  }

  fprintf(stderr, "no context available for interface '%s'\n", node);

  finish:
  freeaddrinfo(result);
  return ctx;
}

static coap_context_t * addContext(char *addr_str, char *port_str)
{
  coap_context_t  *ctx;

  ctx = get_context(addr_str, port_str);
  if (!ctx) {
    printf("Coap-Server:  init error\r\n");
    return NULL;
  }

  init_resources(ctx);

  return ctx;
}

void emberAfPluginCoapServerInitCallback(void)
{
  clock_offset = time(NULL);

  coap_set_log_level(log_level);

  ctx = addContext(addr_str, port_str);
}

void emberAfPluginCoapServerCreateDevice(uint16_t deviceIndex)
{
  if (deviceIndex >= PLUGIN_COAP_SERVER_MAX_PORTS) {
    printf("Coap Server failed to add device %d %d\r\n",
           deviceIndex,
           PLUGIN_COAP_SERVER_MAX_PORTS);
    return;
  }
  if (ctxArray[deviceIndex] == NULL) {
    sprintf(devicePortString, "%04d", BASE_DEVICE_PORT + deviceIndex);

    printf("Creating device %d on port %s\r\n", deviceIndex, devicePortString);
    ctxArray[deviceIndex] = addContext(addr_str, devicePortString);
  }
}

void emberAfPluginCoapServerRemoveDevice(uint16_t deviceIndex)
{
  if (deviceIndex >= PLUGIN_COAP_SERVER_MAX_PORTS) {
    return;
  }
  printf("Removing device %d\r\n", deviceIndex);
  coap_free_context(ctxArray[deviceIndex]);
  ctxArray[deviceIndex] = NULL;
}

int emberAfPluginCoapServerCurrentPort(void)
{
  return currentPort;
}

static void checkContext(coap_context_t  *ctx, int portCount)
{
  FD_ZERO(&readfds);
  FD_SET(ctx->sockfd, &readfds);

  nextpdu = coap_peek_next(ctx);

  coap_ticks(&now);
  while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
    coap_retransmit(ctx, coap_pop_next(ctx) );
    nextpdu = coap_peek_next(ctx);
  }

  if (nextpdu && nextpdu->t <= COAP_RESOURCE_CHECK_TIME) {
    /* set timeout if there is a pdu to send before our automatic timeout occurs */
    tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
    tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
    timeout = &tv;
  } else {
    tv.tv_usec = 0;
    tv.tv_sec = 0; // COAP_RESOURCE_CHECK_TIME;
    timeout = &tv;
  }
  result = select(FD_SETSIZE, &readfds, 0, 0, timeout);

  if (result < 0) {           /* error */
    if (errno != EINTR) {
      perror("select");
    }
  } else if (result > 0) {    /* read from socket */
    if (FD_ISSET(ctx->sockfd, &readfds)) {
      coap_read(ctx);           /* read received data */
    }
  } else {        /* timeout */
    if (time_resource) {
      time_resource->dirty = 1;
    }
  }

#ifndef WITHOUT_OBSERVE
  /* check if we have to send observe notifications */
  coap_check_notify(ctx);
#endif /* WITHOUT_OBSERVE */
}

void emberAfPluginCoapServerTickCallback()
{
  int i;

  currentPort = -1;
  checkContext(ctx, currentPort);

  for (i = 0; i < MAX_PORTS; i++) {
    currentPort = i;
    if (ctxArray[i] != NULL) {
      checkContext(ctxArray[i], currentPort);
    }
  }

  // set the value of the current port to something invalid.
  currentPort = -2;
}

struct coap_packet_t {
  coap_if_handle_t hnd;       /**< the interface handle */
  coap_address_t src;       /**< the packet's source address */
  coap_address_t dst;       /**< the packet's destination address */
  const coap_endpoint_t *interface;

  int ifindex;
  void *session;    /**< opaque session data */

  size_t length;    /**< length of payload */
  unsigned char payload[];  /**< payload */
};

//-----------------------------------------------------------------------------
// Put the TX code here
#include "examples/coap_list.h"

#define MAX_STRING 50
static str server = { 0, NULL };
static uint16_t port = 0;
static char serverStringLocalCopy[MAX_STRING];
static coap_list_t *optlist = NULL;
unsigned int wait_seconds = 90;
static coap_tick_t max_wait;
static int ready = 0;
unsigned int obs_seconds = 30;          /* default observe time */
coap_tick_t obs_wait = 0;               /* timeout for current subscription */
static unsigned char _token_data[8];
str the_token = { 0, _token_data };
static uint8_t messageId = 0;

void emberAfPluginCoapServerSetServerNameAndPort(uint8_t *serverString,
                                                 uint16_t serverPort);

static void possiblySetServerAddress(uint8_t *addr)
{
  uint8_t tempString[MAX_STRING];

  // if there is no server, and the current incoming message is IPv4,
  // fixate on the current incoming address with standard port.
  if (server.length == 0 && addr[10] == 0xff && addr[11] == 0xff) {
    // Set it to current address with port COAP_DEFAULT_PORT (5683).
    sprintf((char *) tempString,
            "%d.%d.%d.%d",
            addr[12],
            addr[13],
            addr[14],
            addr[15]);

    printf("Setting server %s:%d\r\n", tempString, COAP_DEFAULT_PORT);
    emberAfPluginCoapServerSetServerNameAndPort(tempString, COAP_DEFAULT_PORT);
  }
}

void emberAfPluginCoapServerSetServerNameAndPort(uint8_t *serverString,
                                                 uint16_t serverPort)
{
  server.length = strlen((char *) serverString);
  server.s = (uint8_t *) serverStringLocalCopy;

  strncpy(serverStringLocalCopy, (char *) serverString, MAX_STRING);

  port = serverPort;
}

static inline void set_timeout(coap_tick_t *timer, const unsigned int seconds)
{
  coap_ticks(timer);
  *timer += seconds * COAP_TICKS_PER_SECOND;
}

static coap_tid_t clear_obs(coap_context_t *ctx,
                            const coap_endpoint_t *local_interface,
                            const coap_address_t *remote)
{
  coap_pdu_t *pdu;
  coap_list_t *option;
  coap_tid_t tid = COAP_INVALID_TID;
  unsigned char buf[2];

  /* create bare PDU w/o any option  */
  pdu = coap_pdu_init(1,
                      COAP_REQUEST_GET,
                      coap_new_message_id(ctx),
                      COAP_MAX_PDU_SIZE);

  if (!pdu) {
    return tid;
  }

  if (!coap_add_token(pdu, the_token.length, the_token.s)) {
    coap_log(LOG_CRIT, "cannot add token");
    goto error;
  }

  for (option = optlist; option; option = option->next ) {
    coap_option *o = (coap_option *)(option->data);
    if (COAP_OPTION_KEY(*o) == COAP_OPTION_URI_HOST) {
      if (!coap_add_option(pdu,
                           COAP_OPTION_KEY(*o),
                           COAP_OPTION_LENGTH(*o),
                           COAP_OPTION_DATA(*o))) {
        goto error;
      }
      break;
    }
  }

  if (!coap_add_option(pdu,
                       COAP_OPTION_OBSERVE,
                       coap_encode_var_bytes(buf, COAP_OBSERVE_CANCEL),
                       buf)) {
    coap_log(LOG_CRIT, "cannot add option Observe: %u", COAP_OBSERVE_CANCEL);
    goto error;
  }

  for (option = optlist; option; option = option->next ) {
    coap_option *o = (coap_option *)(option->data);
    switch (COAP_OPTION_KEY(*o)) {
      case COAP_OPTION_URI_PORT:
      case COAP_OPTION_URI_PATH:
      case COAP_OPTION_URI_QUERY:
        if (!coap_add_option(pdu,
                             COAP_OPTION_KEY(*o),
                             COAP_OPTION_LENGTH(*o),
                             COAP_OPTION_DATA(*o))) {
          goto error;
        }
        break;
      default:
        ;
    }
  }

  coap_show_pdu(pdu);

  if (pdu->hdr->type == COAP_MESSAGE_CON) {
    tid = coap_send_confirmed(ctx, local_interface, remote, pdu);
  } else {
    tid = coap_send(ctx, local_interface, remote, pdu);
  }

  if (tid == COAP_INVALID_TID) {
    debug("clear_obs: error sending new request");
    coap_delete_pdu(pdu);
  } else if (pdu->hdr->type != COAP_MESSAGE_CON) {
    coap_delete_pdu(pdu);
  }

  return tid;
  error:

  coap_delete_pdu(pdu);
  return tid;
}

static void
close_output(void)
{
}

static int
order_opts(void *a, void *b)
{
  coap_option *o1, *o2;

  if (!a || !b) {
    return a < b ? -1 : 1;
  }

  o1 = (coap_option *)(((coap_list_t *)a)->data);
  o2 = (coap_option *)(((coap_list_t *)b)->data);

  return (COAP_OPTION_KEY(*o1) < COAP_OPTION_KEY(*o2))
         ? -1
         : (COAP_OPTION_KEY(*o1) != COAP_OPTION_KEY(*o2));
}

static coap_pdu_t *
coap_new_request(coap_context_t *ctx,
                 uint8_t m,
                 coap_list_t **options,
                 unsigned char *data,
                 size_t length)
{
  coap_pdu_t *pdu;
  coap_list_t *opt;

  if (!(pdu = coap_new_pdu())) {
    return NULL;
  }

  pdu->hdr->type = COAP_MESSAGE_CON;
  pdu->hdr->id = coap_new_message_id(ctx);
  pdu->hdr->code = m;

  pdu->hdr->token_length = the_token.length;
  if (!coap_add_token(pdu, the_token.length, the_token.s)) {
    debug("cannot add token to request\n");
  }

  coap_show_pdu(pdu);

  if (options) {
    /* sort options for delta encoding */
    LL_SORT((*options), order_opts);

    LL_FOREACH((*options), opt) {
      coap_option *o = (coap_option *)(opt->data);
      coap_add_option(pdu,
                      COAP_OPTION_KEY(*o),
                      COAP_OPTION_LENGTH(*o),
                      COAP_OPTION_DATA(*o));
    }
  }

  coap_add_data(pdu, length, data);

  return pdu;
}

static int resolve_address(const str *server, struct sockaddr *dst)
{
  struct addrinfo *res, *ainfo;
  struct addrinfo hints;
  static char addrstr[256];
  int error, len = -1;

  memset(addrstr, 0, sizeof(addrstr));
  if (server->length) {
    memcpy(addrstr, server->s, server->length);
  } else {
    memcpy(addrstr, "localhost", 9);
  }

  memset((char *)&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;

  error = getaddrinfo(addrstr, NULL, &hints, &res);

  if (error != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    return error;
  }

  for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
    switch (ainfo->ai_family) {
      case AF_INET6:
      case AF_INET:
        len = ainfo->ai_addrlen;
        memcpy(dst, ainfo->ai_addr, len);
        goto finish;
      default:
        ;
    }
  }

  finish:
  freeaddrinfo(res);
  return len;
}

static void messageHandler(struct coap_context_t *ctx,
                           const coap_endpoint_t *local_interface,
                           const coap_address_t *remote,
                           coap_pdu_t *sent,
                           coap_pdu_t *received,
                           const coap_tid_t id UNUSED_PARAM)
{
  printf("incoming message handler...need to check it\r\n");
  ready = 1;
}
coap_list_t *
new_option_node(unsigned short key, unsigned int length, unsigned char *data);

void emberAfPluginCoapServerSendMessage(uint8_t *uri,
                                        uint8_t *payload,
                                        uint16_t length,
                                        uint16_t device,
                                        uint8_t method)
{
  coap_context_t  *ctx = NULL;
  coap_address_t dst;
  fd_set readfds;
  struct timeval tv;
  int result;
  coap_tick_t now;
  coap_queue_t *nextpdu;
  coap_pdu_t  *pdu;
  char port_str[NI_MAXSERV] = "0";
  char node_str[NI_MAXHOST] = "";
  int res;
  coap_tid_t tid = COAP_INVALID_TID;
  coap_list_t *option;
  uint8_t *optPtr;

  // check if server has been set
  if (server.length == 0) {
    // no server set...time to exit
    printf("Warining:  no server set\r\n");
    return;
  }

  // set correct port.
  if (device < MAX_PORTS) {
    sprintf(port_str, "%d", device + BASE_DEVICE_PORT);
  }

  res = resolve_address(&server, &dst.addr.sa);

  if (res < 0) {
    fprintf(stderr, "failed to resolve address\n");
    return;
  }

  dst.size = res;
  dst.addr.sin.sin_port = htons(port);

  /* add Uri-Host if server address differs from uri.host */
  switch (dst.addr.sa.sa_family) {
    case AF_INET:
      printf("AF_INET\r\n");
      /* create context for IPv4 */
      ctx = get_context(node_str[0] == 0 ? "0.0.0.0" : node_str, port_str);
      break;
    case AF_INET6:
      printf("AF_INET 6\r\n");
      /* create context for IPv6 */
      ctx = get_context(node_str[0] == 0 ? "::" : node_str, port_str);
      break;
    default:
      ;
  }

  if (!ctx) {
    coap_log(LOG_EMERG, "cannot create context\n");
    return;
  }

  ctx->message_id = messageId++;

  //coap_register_option(ctx, COAP_OPTION_BLOCK2);
  coap_register_response_handler(ctx, messageHandler);

  /* construct CoAP message */

  optPtr = (uint8_t *) strtok((char *) uri, "/");

  while (optPtr != NULL) {
    option = new_option_node(COAP_OPTION_URI_PATH,
                             strlen((char *) optPtr),
                             optPtr);
    coap_insert(&optlist, option);
    optPtr = (uint8_t *) strtok(NULL, "/");
  }

  option = new_option_node(COAP_OPTION_CONTENT_FORMAT, 1, (uint8_t *) "<");
  coap_insert(&optlist, option);

  if (!(pdu = coap_new_request(ctx, method, &optlist, payload, length))) {
    printf("failed to create PDU\r\n");
    goto CLEANUP;
  }

  if (pdu->hdr->type == COAP_MESSAGE_CON) {
    printf("send confirmable\r\n");
    tid = coap_send_confirmed(ctx, ctx->endpoint, &dst, pdu);
  } else {
    printf("send non-confirmable\r\n");
    tid = coap_send(ctx, ctx->endpoint, &dst, pdu);
  }

  if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID) {
    coap_delete_pdu(pdu);
  }

  set_timeout(&max_wait, wait_seconds);
  debug("timeout is set to %d seconds\n", wait_seconds);

  while (!(ready && coap_can_exit(ctx))) {
    FD_ZERO(&readfds);
    FD_SET(ctx->sockfd, &readfds);

    nextpdu = coap_peek_next(ctx);

    coap_ticks(&now);
    while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
      coap_retransmit(ctx, coap_pop_next(ctx));
      nextpdu = coap_peek_next(ctx);
    }

    if (nextpdu && nextpdu->t < min(obs_wait ? obs_wait : max_wait, max_wait) - now) {
      /* set timeout if there is a pdu to send */
      tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
      tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
    } else {
      /* check if obs_wait fires before max_wait */
      if (obs_wait && obs_wait < max_wait) {
        tv.tv_usec = ((obs_wait - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
        tv.tv_sec = (obs_wait - now) / COAP_TICKS_PER_SECOND;
      } else {
        tv.tv_usec = ((max_wait - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
        tv.tv_sec = (max_wait - now) / COAP_TICKS_PER_SECOND;
      }
    }

    result = select(ctx->sockfd + 1, &readfds, 0, 0, &tv);

    if (result < 0) {   /* error */
      perror("select");
    } else if (result > 0) {  /* read from socket */
      if (FD_ISSET(ctx->sockfd, &readfds)) {
        coap_read(ctx);         /* read received data */
      }
    } else { /* timeout */
      coap_ticks(&now);
      if (max_wait <= now) {
        info("timeout\n");
        break;
      }
      if (obs_wait && obs_wait <= now) {
        debug("clear observation relationship\n");
        clear_obs(ctx, ctx->endpoint, &dst); /* FIXME: handle error case COAP_TID_INVALID */

        /* make sure that the obs timer does not fire again */
        obs_wait = 0;
        obs_seconds = 0;
      }
    }
  }

  CLEANUP:

  close_output();

  coap_delete_list(optlist);
  optlist = NULL;

  coap_free_context(ctx);
}

void emberAfPluginCoapServerSendMessageTick(void)
{
  // do nothing for now.  Need to copy the "while" stuff here.
}

// -----------------------------------------------------
// coap-list.c

/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 * -*- */

/* coap_list.c -- CoAP list structures
 *
 * Copyright (C) 2010,2011,2015 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms of
 * use.
 */

/* #include "coap_config.h" */

#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "mem.h"

coap_list_t *new_option_node(unsigned short key, unsigned int length, unsigned char *data)
{
  coap_list_t *node;

  node = coap_malloc(sizeof(coap_list_t) + sizeof(coap_option) + length);

  if (node) {
    coap_option *option;
    option = (coap_option *)(node->data);
    COAP_OPTION_KEY(*option) = key;
    COAP_OPTION_LENGTH(*option) = length;
    memcpy(COAP_OPTION_DATA(*option), data, length);
  } else {
    coap_log(LOG_DEBUG, "new_option_node: malloc\n");
  }

  return node;
}

int coap_insert(coap_list_t **head, coap_list_t *node)
{
  if (!node) {
    coap_log(LOG_WARNING, "cannot create option Proxy-Uri\n");
  } else {
    /* must append at the list end to avoid re-ordering of
     * options during sort */
    LL_APPEND((*head), node);
  }

  return node != NULL;
}

int coap_delete(coap_list_t *node)
{
  if (node) {
    coap_free(node);
  }
  return 1;
}

void coap_delete_list(coap_list_t *queue)
{
  coap_list_t *elt, *tmp;

  if (!queue) {
    return;
  }

  LL_FOREACH_SAFE(queue, elt, tmp) {
    coap_delete(elt);
  }
}
