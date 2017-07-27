/*
*********************************************************************************************************
*                                              Micrium OS
*                                                 CPU
*
*                             (c) Copyright 2004; Silicon Laboratories Inc.
*                                        https://www.micrium.com
*
*********************************************************************************************************
* Licensing:
*           YOUR USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS OF A MICRIUM SOFTWARE LICENSE.
*   If you are not willing to accept the terms of an appropriate Micrium Software License, you must not
*   download or use this software for any reason.
*   Information about Micrium software licensing is available at https://www.micrium.com/licensing/
*   It is your obligation to select an appropriate license based on your intended use of the Micrium OS.
*   Unless you have executed a Micrium Commercial License, your use of the Micrium OS is limited to
*   evaluation, educational or personal non-commercial uses. The Micrium OS may not be redistributed or
*   disclosed to any third party without the written consent of Silicon Laboratories Inc.
*********************************************************************************************************
* Documentation:
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*********************************************************************************************************
* Technical Support:
*   Support is available for commercially licensed users of Micrium's software. For additional
*   information on support, you can contact info@micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      INTERRUPT CONTROLLER DRIVER
*
*                                                ARMv7-M
*
* File : cpu_int_armv7-m_priv.h
*********************************************************************************************************
* Note(s) : (1) This driver targets the following:
*                 Core : ARMv7M Cortex-M
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                 MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _CPU_INT_ARMV7_M_PRIV_H_
#define  _CPU_INT_ARMV7_M_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_path.h>

#include  "../../../../include/cpu_port_sel.h"

#include  <rtos/common/include/lib_def.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              USAGE GUARD
*********************************************************************************************************
*********************************************************************************************************
*/

#if (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV7_M)


/*
*********************************************************************************************************
*                                      CORE ARM EXCEPTION HANDLERS
*
* Note(s) : (1) These exception handlers are to be used in the initial vector table. All other interrupt
*               handlers will be initialized by CPU_IntInit().
*********************************************************************************************************
*/

void  CPU_IntARMv7_M_HandlerBusFault  (void);
void  CPU_IntARMv7_M_HandlerDefault   (void);
void  CPU_IntARMv7_M_HandlerHardFault (void);
void  CPU_IntARMv7_M_HandlerMemFault  (void);
void  CPU_IntARMv7_M_HandlerNMI       (void);
void  CPU_IntARMv7_M_HandlerUsageFault(void);


/*
*********************************************************************************************************
*                                          CPU INTERRUPT TYPES
*
* Note(s) : (1) The driver implementation MUST define, based on CPU types, the following types:
*
*                   CPU_INT_ID          Type used to identify an interrupt.
*
*                   CPU_INT_PRIO        Type used to hold the priority of an interrupt.
*
*                   CPU_INT_PRIO_RET    Type used to hold the effective priority. Can be nagative.
*
*                   CPU_INT_SENS        Type used to hold the sensitivity (edge, level or pulse) of an
*                                       interrupt.
*
*                   CPU_INT_SRC         Type used to hold the source of a multiplexed interrupt line.
*********************************************************************************************************
*/

typedef  CPU_INT16U  CPU_INT_ID;
typedef  CPU_INT08U  CPU_INT_PRIO;
typedef  CPU_INT16S  CPU_INT_PRIO_RET;
typedef  CPU_INT08U  CPU_INT_SENS;
typedef  CPU_INT08U  CPU_INT_SRC;


/*
*********************************************************************************************************
*                                      CPU INTERRUPT CONFIGURATION
*
* Note(s) : (1) The driver implementation MUST define the following configuration parameters:
*
*                   CPU_INT_HANDLER_HAS_ARG     Indicates whether a handler can receive an argument.
*
*                   CPU_INT_NBR_OF_INT          Indicates the number of interrupts the controller supports.
*********************************************************************************************************
*/

#define  CPU_INT_HANDLER_HAS_ARG                        DEF_NO

                                                                /* Based on the selected MCU, determine number of     */
                                                                /* supported interrupts.                              */
#if   ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M3) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M4) || \
       (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M7))
#define  CPU_INT_NBR_OF_INT                             256u
#define  CPU_INT_TABLE_ALIGN                            512
#else
#define  CPU_INT_NBR_OF_INT                             496u
#define  CPU_INT_TABLE_ALIGN                            1024
#endif


/*
*********************************************************************************************************
*                                                DEFINES
*
* Note(s) : (1) These defines should only be used by code which explicitly requires this interrupt
*               controller.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              DATA TYPES
*
* Note(s) : (1) These data types should only be used by code which explicitly requires this interrupt
*               controller.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* RTOS_INT_CONTROLLER_NAME                             */

#endif                                                          /* _CPU_INT_ARMV7_M_PRIV_H_                             */
