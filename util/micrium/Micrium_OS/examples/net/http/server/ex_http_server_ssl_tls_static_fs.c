/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing:
*   The licensor of this EXAMPLE CODE is Silicon Laboratories Inc.
*
*   Silicon Laboratories Inc. grants you a personal, worldwide, royalty-free, fully paid-up license to
*   use, copy, modify and distribute the EXAMPLE CODE software, or portions thereof, in any of your
*   products.
*
*   Your use of this EXAMPLE CODE is at your own risk. This EXAMPLE CODE does not come with any
*   warranties, and the licensor disclaims all implied warranties concerning performance, accuracy,
*   non-infringement, merchantability and fitness for your application.
*
*   The EXAMPLE CODE is provided "AS IS" and does not come with any support.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: https://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                        HTTP SERVER INSTANCE SSL TLS WITH STATIC FS EXAMPLE
*
* File : ex_http_server_ssl_tls_static_fs.c
*********************************************************************************************************
* Note(s) : (1) This example shows how to initialize Micrium OS HTTP Server, initialize a web
*               server instance and start it.
*
*           (2) This example does not require a File System.
*
*           (3) This file is an example about how to use the HTTP Server, It may not cover all case
*               needed by a real application. Also some modification might be needed, insert the
*               code to perform the stated actions wherever 'TODO' comments are found.
*
*               (a) For instance this example doesn't manage the link state (plugs and unplugs),
*                   this can be a problem when switching from a network to another network.
*
*               (b) This example is not fully tested, so it is not guaranteed that all cases are
*                   covered properly.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>
#include  <rtos/common/include/lib_str.h>

#if (defined(RTOS_MODULE_NET_HTTP_SERVER_AVAIL) && \
     defined(RTOS_MODULE_NET_SSL_TLS_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/rtos_path.h>
#include  <rtos/net/include/http_server.h>
#include  <rtos/net/include/http_server_fs_port_static.h>
#include  <rtos/net/include/net_cfg_net.h>

#include  "ex_http_server.h"
#include  "ex_http_server_hooks.h"
#include  "files/_404_html.h"
#include  "files/form_html.h"
#include  "files/index_html.h"
#include  "files/list_html.h"
#include  "files/login_html.h"
#include  "files/logo_gif.h"
#include  "files/uc_style_css.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_HTTP_Server_StaticFS_Prepare (void);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                        SECURE CONFIGURATION
* Note(s) : (1) SSL/TLS certificate and key can be acquired either:
*
*               (a) From a certificate authority. Acquiring the certificate from an authority should ensure to
*                   avoid the untrusted warning message to be displayed when accessing the web server.
*
*               (b) Generated from a SSL tool such as OpenSSL. This kind of tool generate self-signed certificate
*                   and the untrusted warning message will be displayed every time the web server is accessed.
*
*           (2) Format of the key and certificate. Supported formats are PEM and DER. The value can either be:
*                   NET_SOCK_SECURE_CERT_KEY_FMT_PEM
*                   NET_SOCK_SECURE_CERT_KEY_FMT_DER
*
*               Note that if the PEM format is used, do not include the  -----BEGIN CERTIFICATE----- ,  -----END
*               CERTIFICATE----- ,  -----BEGIN RSA PRIVATE KEY-----  or  -----END RSA PRIVATE KEY-----
*               sections.
*
*           (3) The HTTPs_SECURE_CFG structure referenced in argument must exist throughout the lifetime of the
*               HTTPs server since the certificate and the key are not copied internally and are directly
*               referenced throughout the HTTPs_SECURE_CFG pointer.
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                        /* See Note #1.                                 */
#define HTTPs_CFG_SECURE_CERT                                    \
"MIIEEjCCAvqgAwIBAgIBBzANBgkqhkiG9w0BAQUFADAaMRgwFgYDVQQDEw9WYWxp\
Y29yZS1EQzEtQ0EwHhcNMTEwMzE4MTcwMTQyWhcNMjEwMzE1MTcwMTQyWjCBkDEL\
MAkGA1UEBhMCVVMxCzAJBgNVBAgTAkNBMQ8wDQYDVQQHEwZJcnZpbmUxHjAcBgNV\
BAoTFVZhbGljb3JlIFRlY2hub2xvZ2llczEhMB8GA1UEAxMYbGFuLWZ3LTAxLnZh\
bGljb3JlLmxvY2FsMSAwHgYJKoZIhvcNAQkBFhFhZG1pbkBsb2NhbGRvbWFpbjCC\
ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALwGOahytiwshzz1s/ngxy1+\
+VrXZYjKSEzMYbJCUhK9xA5fz8pGtOZIXI+CasZPSbXv+ZDLGpSpeFnOL49plYRs\
vmTxg2n3AlZbP6pD9OPU8rmufsTvXAmQGxxIkdmWiXYJk0pbj+U698me6DKMV/sy\
3ekQaQC2I2nr8uQw8RhuNhhlkWyjBWdXnS2mLNLSan2Jnt8rumtAi3B+vF5Vf0Fa\
kLJNt45R0f5jjuab+qw4PKMZEQbqe0XTNzkxdD0XNRBdKlajffoZPBJ7xkfuKUA3\
cMjXKzetABoKvsv+ElfvqlrI9RXvTXy52EaQmVhiOyBHrScq4RbwtDQsd59Qmk0C\
AwEAAaOB6zCB6DAJBgNVHRMEAjAAMBEGCWCGSAGG+EIBAQQEAwIGQDA0BglghkgB\
hvhCAQ0EJxYlRWFzeS1SU0EgR2VuZXJhdGVkIFNlcnZlciBDZXJ0aWZpY2F0ZTAd\
BgNVHQ4EFgQUrq5KF11M9rpKm75nAs+MaiK0niYwUQYDVR0jBEowSIAU2Q9eGjzS\
LZhvlRRKO6c4Q5ATtuChHqQcMBoxGDAWBgNVBAMTD1ZhbGljb3JlLURDMS1DQYIQ\
T9aBcT0uXoxJmC0ohp7oSTATBgNVHSUEDDAKBggrBgEFBQcDATALBgNVHQ8EBAMC\
BaAwDQYJKoZIhvcNAQEFBQADggEBAAUMm/9G+mhxVIYK4anc34FMqu88NQy8lrh0\
loNfHhIEKnerzMz+nQGidf+KBg5K5U2Jo8e9gVnrzz1gh2RtUFvDjgosGIrgYZMN\
yreNUD2I7sWtuWFQyEuewbs8h2MECs2xVktkqp5KPmJGCYGhXbi+zuqi/19cIsly\
yS01kmexwcFMXyX4YOVbG+JFHy1b4zFvWgSDULj14AuKfc8RiZNvMRMWR/Jqlpr5\
xWQRSmkjuzQMFavs7soZ+kHp9vnFtY2D6gF2cailk0sdG0uuyPBVxEJ2meifG6eb\
o3FQzdtIrB6oMFHEU00P38SJq+mrDItPDRXNLa2Nrtc1EJtmjws="

                                                                        /* See Note #1.                                 */
#define HTTPs_CFG_SECURE_KEY                                     \
"MIIEogIBAAKCAQEAvAY5qHK2LCyHPPWz+eDHLX75WtdliMpITMxhskJSEr3EDl/P\
yka05khcj4Jqxk9Jte/5kMsalKl4Wc4vj2mVhGy+ZPGDafcCVls/qkP049Tyua5+\
xO9cCZAbHEiR2ZaJdgmTSluP5Tr3yZ7oMoxX+zLd6RBpALYjaevy5DDxGG42GGWR\
bKMFZ1edLaYs0tJqfYme3yu6a0CLcH68XlV/QVqQsk23jlHR/mOO5pv6rDg8oxkR\
Bup7RdM3OTF0PRc1EF0qVqN9+hk8EnvGR+4pQDdwyNcrN60AGgq+y/4SV++qWsj1\
Fe9NfLnYRpCZWGI7IEetJyrhFvC0NCx3n1CaTQIDAQABAoIBAEbbqbr7j//RwB2P\
EwZmWWmh4mMDrbYBVYHrvB2rtLZvYYVxQiOexenK92b15TtbAhJYn5qbkCbaPwrJ\
E09eoQRI3u+3vKigd/cHaFTIS2/Y/qhPRGL/OZY5Ap6EEsMHYkJjlWh+XRosQNlw\
01zJWxbFsq90ib3E5k+ypdStRQ7JQ9ntvDAP6MDp3DF2RYf22Tpr9t3Oi2mUirOl\
piOEB55wydSyIhSHusbms3sp2uvQBYJjZP7eENEQz55PebTzl9UF2dgJ0wJFS073\
rvp46fibcch1L7U6v8iUNaS47GTs3MMyO4zda73ufhYwZLU5gL8oEDY3tf/J8zuC\
mNurr0ECgYEA8i1GgstYBFSCH4bhd2mLu39UVsIvHaD38mpJE6avCNOUq3Cyz9qr\
NzewG7RyqR43HsrVqUSQKzlAGWqG7sf+jkiam3v6VW0y05yqDjs+SVW+ZN5CKyn3\
sMZV0ei4MLrfxWneQaKy/EUTJMlz3rLSDM/hpJoA/gOo9BIFRf2HPkkCgYEAxsGq\
LYU+ZEKXKehVesh8rIic4QXwzeDmpMF2wTq6GnFq2D4vWPyVGDWdORcIO2BojDWV\
EZ8e7F2SghbmeTjXGADldYXQiQyt4Wtm+oJ6d+/juKSrQ1HIPzn1qgXDNLPfjd9o\
9lX5lGlRn49Jrx/kKQAPTcnCa1IirIcsmcdiy+UCgYBEbOBwUi3zQ0Fk0QJhb/Po\
LSjSPpl7YKDN4JP3NnBcKRPngLc1HU6lElny6gA/ombmj17hLZsia1GeHMg1LVLS\
NtdgOR5ZBrqGqcwuqzSFGfHqpBXEBl6SludmoL9yHUreh3QhzWuO9aFcEoNnl9Tb\
g9z4Wf8Pxk71byYISYLt6QKBgERActjo3ZD+UPyCHQBp4m45B246ZQO9zFYdXVNj\
gE7eTatuR0IOkoBawN++6gPByoUDTWpcsvjF9S6ZAJH2E97ZR/KAfijh4r/66sTx\
k26mQRPB8FHQvqv/kj3NdsgdUJJeeqPEyEzPkcjyIoJxuB7gN2El/I5wCRon3Qf9\
sQ6FAoGAfVOaROSAtq/bq9JIL60kkhA9sr3KmX52PnOR2hW0caWi96j+2jlmPT93\
4A2LIVUo6hCsHLSCFoWWiyX9pIqyYTn5L1EmeBO0+E8BH9F/te9+ZZ53U+quwc/X\
AZ6Pseyhj7S9wkI5hZ9SO1gcK4rWrAK/UFOIzzlACr5INr723vw="


#define  HTTPs_CFG_SECURE_CERT_LEN      (sizeof(HTTPs_CFG_SECURE_CERT) - 1)
#define  HTTPs_CFG_SECURE_KEY_LEN       (sizeof(HTTPs_CFG_SECURE_KEY)  - 1)


/*
*********************************************************************************************************
*                                      HTTP SERVER HOOK CONFIGURATION
*
* Note(s): (1)  When the instance is created, an hook function can be called to initialize connection objects used by the instance.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_InstanceInitHook() function for further details.
*
*          (2)  Each time a header field other than the default one is received, a hook function is called
*               allowing to choose which header field(s) to keep for further processing.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqHdrRxHook() function for further details.
*
*          (3)  For each new incoming connection request a hook function can be called by the web server to authenticate
*               the remote connection to accept or reject it. This function can have access to allow stored request header
*               field.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqHook() function for further details.
*
*          (4)  If the upper application want to parse the data received in the request body itself, a hook function is available.
*               It will be called each time new data are received. The exception is when a POST request with a form is
*               received. In that case, the HTTP server core will parse the body and saved the data into Key-Value blocks.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqBodyRxHook() function for further details.
*
*          (5)  The Signal hook function occurs after the HTTP request has been completely received.
*               The hook function SHOULD NOT be blocking and SHOULD return quickly. A time consuming function will
*               block the processing of the other connections and reduce the HTTP server performance.
*               In case the request processing is time consuming, the Poll hook function SHOULD be enabled to
*               allow the server to periodically verify if the upper application has finished the request processing.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ReqRdySignalHook() function for further details.
*
*          (6)  The Poll hook function SHOULD be enable in case the request processing require lots of time. It
*               allows the HTTP server to periodically poll the upper application and verify if the request processing
*               has finished.
*               If the Poll feature is not required, this field SHOULD be set as DEF_NULL.
*               See HTTPs_ReqRdyPollHook() function for further details.
*
*          (7)  Before an HTTP response message is transmitted, a hook function is called to enable adding header field(s) to
*               the message before it is sent.
*               The Header Module must be enabled for this hook to be called. See HTTPs_CFG_HDR in http-s_cfg.h.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_RespHdrTxHook() function for further details.
*
*          (8)  The hook function is called by the web server when a token is found. This means the hook
*               function must fill a buffer with the value of the instance token to be sent.
*               If the feature is not enabled, this field is not used and can be set as DEF_NULL.
*               See 'HTTPs_RespTokenValGetHook' for further information.
*
*          (9)  To allow the upper application to transmit data with the Chunked Transfer Encoding, a hook function is
*               available. If defined, it will be called at the moment of the Response body transfer, and it will be called
*               until the application has transfer all its data.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_RespChunkDataGetHook() function for further details.
*
*          (10) Once an HTTP transaction is completed, a hook function can be called to notify the upper application that the
*               transaction is done. This hook function could be used to free some previously allocated memory.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_TransCompleteHook() function for further details.
*
*          (11) When an internal error occurs during the processing of a connection a hook function can be called to
*               notify the application of the error and to change the behavior such as the status code and the page returned.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ErrHook() function for further details.
*
*          (12) Get error file hook can be called every time an error has occurred when processing a connection.
*               This function can set the web page that should be transmit instead of the default error page defined
*               in http-s_cfg.h.
*               If set to DEF_NULL the default error page will be used for every error.
*               See HTTPs_ErrFileGetHook() function for further details.
*
*          (13) Once a connection is closed a hook function can be called to notify the upper application that a connection
*               is no more active. This hook function could be used to free some previously allocated memory.
*               If the hook is not required by the upper application, it can be set as DEF_NULL and no function will be called.
*               See HTTPs_ConnCloseHook() function for further details.
*********************************************************************************************************
*/

const  HTTPs_HOOK_CFG  Ex_HTTP_Server_HooksSSL_TLS =
{
    .OnInstanceInitHook  = Ex_HTTP_Server_InstanceInitHook,     /* .OnInstanceInitHook    See Note #1.                  */
    .OnReqHdrRxHook      = Ex_HTTP_Server_ReqHdrRxHook,         /* .OnReqHdrRxHook        See Note #2.                  */
    .OnReqHook           = Ex_HTTP_Server_ReqHookNoREST,        /* .OnReqHook             See Note #3.                  */
    .OnReqBodyRxHook     = Ex_HTTP_Server_ReqBodyRxHook,        /* .OnReqBodyRxHook       See Note #4.                  */
    .OnReqRdySignalHook  = Ex_HTTP_Server_ReqRdySignalHook,     /* .OnReqRdySignalHook    See Note #5.                  */
    .OnReqRdyPollHook    = Ex_HTTP_Server_ReqRdyPollHook,       /* .OnReqRdyPollHook      See Note #6.                  */
    .OnRespHdrTxHook     = Ex_HTTP_Server_RespHdrTxHook,        /* .OnRespHdrTxHook       See Note #7.                  */
    .OnRespTokenHook     = Ex_HTTP_Server_RespTokenValGetHook,  /* .OnRespTokenHook       See Note #8.                  */
    .OnRespChunkHook     = Ex_HTTP_Server_RespChunkDataGetHook, /* .OnRespChunkHook       See Note #9.                  */
    .OnTransCompleteHook = Ex_HTTP_Server_TransCompleteHook,    /* .OnTransCompleteHook   See Note #10.                 */
    .OnErrHook           = Ex_HTTP_Server_ErrHook,              /* .OnErrHook             See Note #11.                 */
    .OnErrFileGetHook    = Ex_HTTP_Server_ErrFileGetHook,       /* .OnErrFileGetHook      See Note #12.                 */
    .OnConnCloseHook     = Ex_HTTP_Server_ConnCloseHook         /* .OnConnCloseHook       See Note #13.                 */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        HTTP_ServerBasicInit()
*
* Description : Initializes and starts a basic web server instance. Retrieves WebServer files built-in
*               static file system.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) HTTP server instance configurations content MUST remain persistent. You can also use
*                   global variable or constants.
*
*               (2) Prior to do any call to HTTP module, it must be initialized. This is done by
*                   calling HTTPs_Init(). If the process is successful, the Web server internal data structures are
*                   initialized.
*
*               (3) Each web server must be initialized before it can be started or stopped. HTTPs_InstanceInit()
*                   is responsible to allocate memory for the instance, initialize internal data structure and
*                   create the web server instance's task.
*
*                   (a) The first argument is the instance configuration, which should be modified following you
*                       requirements. The intance's configuration set the server's port, the number of connection that
*                       can be accepted, the hooks functions, etc.
*
*                   (b) The second argument is the pointer to the instance task configuration. It sets the task priority,
*                       the stack size of the task, etc.
*
*               (4) Once a web server instance is initialized, it can be started using HTTPs_InstanceStart() to
*                   become come accessible. This function starts the web server instance's task. Each instance has
*                   is own task and all accepted connection is processed with this single task.
*
*                   At this point you should be able to access your web server instance using the following
*                   address in your favorite web browser:
*
*                       http://<target_ip_address>
*********************************************************************************************************
*/

void  Ex_HTTP_Server_InstanceCreateSecure (void)
{
    RTOS_TASK_CFG   *p_cfg_task;
    HTTPs_CFG       *p_cfg_http;
    HTTPs_INSTANCE  *p_instance;
    RTOS_ERR         err;


    Ex_HTTP_Server_StaticFS_Prepare();                          /* Add files to the static file system.                 */

                                                                /* ---------- ALLOC CFG STRUCT (SEE NOTE 1) ----------- */
    p_cfg_task = (RTOS_TASK_CFG *)Mem_SegAlloc("Ex HTTP Server task cfg struct",
                                                DEF_NULL,
                                                sizeof(RTOS_TASK_CFG),
                                               &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http = (HTTPs_CFG *)Mem_SegAlloc("Ex HTTP Server cfg struct",
                                            DEF_NULL,
                                            sizeof(HTTPs_CFG),
                                           &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->FS_CfgPtr = Mem_SegAlloc("Ex HTTP Server FS cfg struct",
                                          DEF_NULL,
                                          sizeof(HTTPs_CFG_FS_STATIC),
                                         &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->HdrRxCfgPtr = (HTTPs_HDR_RX_CFG *)Mem_SegAlloc("Ex HTTP Server hdr rx cfg struct",
                                                                DEF_NULL,
                                                                sizeof(HTTPs_HDR_RX_CFG),
                                                               &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->HdrTxCfgPtr = (HTTPs_HDR_TX_CFG *)Mem_SegAlloc("Ex HTTP Server hdr tx cfg struct",
                                                                DEF_NULL,
                                                                sizeof(HTTPs_HDR_TX_CFG),
                                                               &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->QueryStrCfgPtr = (HTTPs_QUERY_STR_CFG *)Mem_SegAlloc("Ex HTTP Server query str cfg struct",
                                                                      DEF_NULL,
                                                                      sizeof(HTTPs_QUERY_STR_CFG),
                                                                     &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->FormCfgPtr = (HTTPs_FORM_CFG *)Mem_SegAlloc("Ex HTTP Server form cfg struct",
                                                              DEF_NULL,
                                                              sizeof(HTTPs_FORM_CFG),
                                                             &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->TokenCfgPtr = (HTTPs_TOKEN_CFG *)Mem_SegAlloc("Ex HTTP Server token cfg struct",
                                                              DEF_NULL,
                                                              sizeof(HTTPs_TOKEN_CFG),
                                                             &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    p_cfg_http->SecurePtr = (HTTPs_SECURE_CFG *)Mem_SegAlloc("Ex HTTP Server secure cfg struct",
                                                              DEF_NULL,
                                                              sizeof(HTTPs_SECURE_CFG),
                                                             &err);



                                                                /* ---------------- PREPARE CFG STRUCT ---------------- */
                                                                /* Server instance task cfg.                            */
    p_cfg_task->Prio            = 21u;
    p_cfg_task->StkSizeElements = 1024u;
    p_cfg_task->StkPtr          = DEF_NULL;

                                                                /* Server instance cfg.                                 */
   ((HTTPs_HDR_RX_CFG    *)p_cfg_http->HdrRxCfgPtr)->NbrPerConnMax               =  15u;
   ((HTTPs_HDR_RX_CFG    *)p_cfg_http->HdrRxCfgPtr)->DataLenMax                  =  128u;
   ((HTTPs_HDR_TX_CFG    *)p_cfg_http->HdrTxCfgPtr)->NbrPerConnMax               =  15u;
   ((HTTPs_HDR_TX_CFG    *)p_cfg_http->HdrTxCfgPtr)->DataLenMax                  =  128u;
   ((HTTPs_QUERY_STR_CFG *)p_cfg_http->QueryStrCfgPtr)->NbrPerConnMax            =  5u;
   ((HTTPs_QUERY_STR_CFG *)p_cfg_http->QueryStrCfgPtr)->KeyLenMax                =  15u;
   ((HTTPs_QUERY_STR_CFG *)p_cfg_http->QueryStrCfgPtr)->ValLenMax                =  20u;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->NbrPerConnMax                =  15u;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->KeyLenMax                    =  10u;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->ValLenMax                    =  48u;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->MultipartEn                  =  DEF_ENABLED;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->MultipartFileUploadEn        =  DEF_DISABLED;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->MultipartFileUploadOverWrEn  =  DEF_DISABLED;
   ((HTTPs_FORM_CFG      *)p_cfg_http->FormCfgPtr)->MultipartFileUploadFolderPtr =  DEF_NULL;
   ((HTTPs_TOKEN_CFG     *)p_cfg_http->TokenCfgPtr)->NbrPerConnMax               =  5u;
   ((HTTPs_TOKEN_CFG     *)p_cfg_http->TokenCfgPtr)->ValLenMax                   =  12u;
   ((HTTPs_CFG_FS_STATIC *)p_cfg_http->FS_CfgPtr)->FS_API_Ptr                    = &HTTPs_FS_API_Static;
   ((HTTPs_SECURE_CFG    *)p_cfg_http->SecurePtr)->CertPtr                       =  HTTPs_CFG_SECURE_CERT;
   ((HTTPs_SECURE_CFG    *)p_cfg_http->SecurePtr)->CertLen                       =  HTTPs_CFG_SECURE_CERT_LEN;
   ((HTTPs_SECURE_CFG    *)p_cfg_http->SecurePtr)->KeyPtr                        =  HTTPs_CFG_SECURE_KEY;
   ((HTTPs_SECURE_CFG    *)p_cfg_http->SecurePtr)->KeyLen                        =  HTTPs_CFG_SECURE_KEY_LEN;
   ((HTTPs_SECURE_CFG    *)p_cfg_http->SecurePtr)->Fmt                           =  NET_SOCK_SECURE_CERT_KEY_FMT_PEM;
   ((HTTPs_SECURE_CFG    *)p_cfg_http->SecurePtr)->CertChain                     =  DEF_NO;
    p_cfg_http->OS_TaskDly_ms                                                    =  0u;
    p_cfg_http->Port                                                             =  443u;
    p_cfg_http->ConnNbrMax                                                       =  15u;
    p_cfg_http->ConnInactivityTimeout_s                                          =  15u;
    p_cfg_http->BufLen                                                           =  1460u;
    p_cfg_http->ConnPersistentEn                                                 =  DEF_ENABLED;
    p_cfg_http->FS_Type                                                          =  HTTPs_FS_TYPE_STATIC;
    p_cfg_http->PathLenMax                                                       =  255u;
    p_cfg_http->DfltResourceNamePtr                                              = "/index.html";
    p_cfg_http->HostNameLenMax                                                   =  128u;
    p_cfg_http->HooksPtr                                                         = &Ex_HTTP_Server_HooksSSL_TLS;
    p_cfg_http->Hooks_CfgPtr                                                     =  DEF_NULL;

    Str_Copy_N(&Ex_HTTPs_Name[0], "Secure with Static FS", 60);

    p_instance = HTTPs_InstanceInit(p_cfg_http,                 /* Instance configuration. See Note #3a.                */
                                    p_cfg_task,                 /* Instance task configuration. See Note #3b.           */
                                   &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* ------------ START WEB SERVER INSTANCE ------------- */
    HTTPs_InstanceStart(p_instance,                             /* Instance handle. See Note #4.                        */
                       &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  Ex_HTTP_Server_StaticFS_Prepare()
*
* Description : Example of web files add to static file system.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  Ex_HTTP_Server_StaticFS_Prepare (void)
{
    CPU_BOOLEAN  success;


    success = HTTPs_FS_Init();                                  /* Initialize static FS.                                */
    APP_RTOS_ASSERT_DBG((success == DEF_OK), ;);

                                                                /* Add files to static FS.                              */
    success = HTTPs_FS_AddFile(_404_HTML_NAME,
                               _404_HTML_CONTENT,
                               _404_HTML_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);

    success = HTTPs_FS_AddFile(FORM_HTML_NAME,
                               FORM_HTML_CONTENT,
                               FORM_HTML_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);

    success = HTTPs_FS_AddFile(INDEX_HTML_NAME,
                               INDEX_HTML_CONTENT,
                               INDEX_HTML_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);

    success = HTTPs_FS_AddFile(LIST_HTML_NAME,
                               LIST_HTML_CONTENT,
                               LIST_HTML_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);

    success = HTTPs_FS_AddFile(LOGIN_HTML_NAME,
                               LOGIN_HTML_CONTENT,
                               LOGIN_HTML_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);

    success = HTTPs_FS_AddFile(LOGO_GIF_NAME,
                               LOGO_GIF_CONTENT,
                               LOGO_GIF_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);

    success = HTTPs_FS_AddFile(UC_STYLE_CSS_NAME,
                               UC_STYLE_CSS_CONTENT,
                               UC_STYLE_CSS_SIZE);
    APP_RTOS_ASSERT_CRITICAL(success == DEF_OK, ;);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* RTOS_MODULE_NET_HTTP_SERVER_AVAIL && RTOS_MODULE_NET_SSL_TLS_AVAIL */

