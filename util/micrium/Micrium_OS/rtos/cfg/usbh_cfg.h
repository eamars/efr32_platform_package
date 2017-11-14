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
*                                        USB host Configuration
*
*                                      CONFIGURATION TEMPLATE FILE
*
* Filename      : usbh_cfg.h
* Programmer(s) : Micrium
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _USBH_CFG_H_
#define  _USBH_CFG_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/lib_def.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      OPTIMIZATION CONFIGURATION
*
* Note(s) : (1) USB Host product can be configured to be optimized for speed and determinism (requires
*               extra configurations) or for reduced RAM usage.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_OPTIMIZE_SPD_EN                           DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       ALLOCATION CONFIGURATION
*
* Note(s) : (1) USB Host product must allocate many structures for each connected devices. The quantity
*               of structures greatly depends on the devices. It can either allocate a defined quantity
*               of each type of structure at initialization (requires extra configurations) or allocate
*               them as devices get connected.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_INIT_ALLOC_EN                             DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                              ALTERNATE INTERFACE SUPPORT CONFIGURATION
*
* Note(s) : (1) USB Host can be configured to support alternate interfaces (settings) or not.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_ALT_IF_EN                                 DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     STRING SUPPORT CONFIGURATION
*
* Note(s) : (1) USB Host can be configured to support USB strings or not.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_STR_EN                                    DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                  STACK UNINIT SUPPORT CONFIGURATION
*
* Note(s) : (1) USB Host supports uninitialization. This allows to clear the memory segments assigned to
*               USB Host and assigned them to other purposes.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_UNINIT_EN                                 DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                    INFORMATION FIELD ENABLE MASK
*
* Note(s) : (1) USB Host can be configured to save only some information field about objects like
*               devices, configurations, functions, etc. Setting USBH_CFG_FIELD_EN_MASK to
*               USBH_CFG_FIELD_EN_ALL will enable all the available fields.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_FIELD_EN_MASK                             USBH_CFG_FIELD_EN_ALL


/*
*********************************************************************************************************
*********************************************************************************************************
*                               PERIODIC TRANSFERS SUPPORT CONFIGURATION
*
* Note(s) : (1) USB Host can be configured to support periodic transfers (interrupt and isochronous) or
*               not. Periodic transfers are required by many of the class drivers. Support for external
*               hubs MUST be disabled to be able to disable periodic transfers.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CFG_PERIODIC_XFER_EN                          DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                  EXTERNAL HUB SUPPORT CONFIGURATION
*
* Note(s) : (1) USB Host can be configured to support external hubs or not.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_HUB_CFG_EXT_HUB_EN                            DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     CDC ACM CLASS CONFIGURATIONS
*
* Note(s) : (1) CDC ACM normally queries periodically the serial status. This can be disabled to disable
*               support for periodic transfers.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_CDC_CFG_NOTIFICATIONS_RX_EN                   DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     USB2SER CLASS CONFIGURATIONS
*
* Note(s) : (1) Some adapter drivers query the serial status periodically. This can be disabled.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  USBH_USB2SER_CFG_ADAPTER_CAPABILITIES_CHK_EN       DEF_ENABLED

#define  USBH_USB2SER_CFG_NOTIFICATIONS_RX_EN               DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of usbh_cfg.h module include.                    */
