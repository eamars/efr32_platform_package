/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Kernel
*
*                             (c) Copyright 2009; Silicon Laboratories Inc.
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
*                                          PRIORITY MANAGEMENT
*
* File : os_prio.c
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

#if (defined(RTOS_MODULE_KERNEL_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include "../include/os.h"
#include "os_priv.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_prio__c = "$Id: $";
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_DATA   OSPrioTbl[OS_PRIO_TBL_SIZE];                         /* Declare the array local to this file to allow for... */
                                                                /* ... optimization. This allows the table to be    ... */
                                                                /* ... located in fast memory.                          */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          INTERNAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             OS_PrioInit()
*
* Description : This function is called by the Kernel to initialize the list of ready priorities.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
*********************************************************************************************************
*/

void  OS_PrioInit (void)
{
    CPU_DATA  i;


                                                                /* Clear the bitmap table ... no task is ready          */
    for (i = 0u; i < OS_PRIO_TBL_SIZE; i++) {
         OSPrioTbl[i] = 0u;
    }

#if (OS_CFG_TASK_IDLE_EN == DEF_DISABLED)
    OS_PrioInsert ((OS_CFG_PRIO_MAX - 1u));                     /* Insert what would be the idle task                   */
#endif
}


/*
*********************************************************************************************************
*                                          OS_PrioGetHighest()
*
* Description : This function is called by other kernel services to determine the highest priority task
*               waiting on the event.
*
* Argument(s) : None.
*
* Return(s)   : The priority of the Highest Priority Task (HPT) waiting for the event.
*
* Note(s)     : (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
*********************************************************************************************************
*/

OS_PRIO  OS_PrioGetHighest (void)
{
    CPU_DATA  *p_tbl;
    OS_PRIO    prio;


    prio  = 0u;
    p_tbl = &OSPrioTbl[0];
#if (OS_CFG_PRIO_MAX > DEF_INT_CPU_NBR_BITS)
    while (*p_tbl == 0u) {                                      /* Search the bitmap table for the highest priority     */
        prio += DEF_INT_CPU_NBR_BITS;                           /* Compute the step of each CPU_DATA entry              */
        p_tbl++;
    }
#endif
    prio += (OS_PRIO)CPU_CntLeadZeros(*p_tbl);                  /* Find the position of the first bit set at the entry  */

    return (prio);
}


/*
*********************************************************************************************************
*                                            OS_PrioInsert()
*
* Description : This function is called to insert a priority in the priority table.
*
* Argument(s) : prio    The priority to insert.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
*********************************************************************************************************
*/

void  OS_PrioInsert (OS_PRIO  prio)
{
    CPU_DATA  bit;
    CPU_DATA  bit_nbr;
    OS_PRIO   ix;

#if (OS_CFG_PRIO_MAX > DEF_INT_CPU_NBR_BITS)
    ix             = prio / DEF_INT_CPU_NBR_BITS;
    bit_nbr        = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);
#else
    ix             = 0u;
    bit_nbr        = prio;
#endif
    bit            = 1u;
    bit          <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
    OSPrioTbl[ix] |= bit;
}


/*
*********************************************************************************************************
*                                            OS_PrioRemove()
*
* Description : This function is called to remove a priority in the priority table.
*
* Argument(s) : prio    The priority to remove.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
*********************************************************************************************************
*/

void  OS_PrioRemove (OS_PRIO  prio)
{
    CPU_DATA  bit;
    CPU_DATA  bit_nbr;
    OS_PRIO   ix;

#if (OS_CFG_PRIO_MAX > DEF_INT_CPU_NBR_BITS)
    ix             = prio / DEF_INT_CPU_NBR_BITS;
    bit_nbr        = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);
#else
    ix             = 0u;
    bit_nbr        = prio;
#endif
    bit            = 1u;
    bit          <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
    OSPrioTbl[ix] &= ~bit;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* (defined(RTOS_MODULE_KERNEL_AVAIL)) */
