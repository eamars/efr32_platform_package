/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*********************************************************************************************************
* Licensing terms:
*   This file is provided as an example on how to use Micrium products. It has not necessarily been
*   tested under every possible condition and is only offered as a reference, without any guarantee.
*
*   Please feel free to use any application code labeled as 'EXAMPLE CODE' in your application products.
*   Example code may be used as is, in whole or in part, or may be used as a reference only. This file
*   can be modified as required.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: http://www.micrium.com
*
*   Please help us continue to provide the Embedded community with the finest software available.
*
*   Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                              BSP MODULE
*
* File : bsp_os.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _BSP_OS_H_
#define  _BSP_OS_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/toolchains.h>
#include  <rtos/common/include/rtos_path.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*
* Note(s) : (1) A bug exists in some version of IAR where the compiler can make some optimizations using
*               a weak definition even though a strong definition exists somewhere else. Using the
*               volatile type qualifier in the weak definition prevents this optimization.
*********************************************************************************************************
*********************************************************************************************************
*/

#if (RTOS_TOOLCHAIN != RTOS_TOOLCHAIN_IAR)
#define  BSP_HW_INFO_EXT(type, name)  PP_WEAK(type, name, {0})
#else
                                                                /* See note (1).                                        */
#define  BSP_HW_INFO_EXT(type, name)  PP_WEAK(volatile type, name, {0})
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

void  BSP_SystemInit(void);

void  BSP_TickInit  (void);

void  BSP_OS_Init    (void);

#ifdef __cplusplus
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of module include.                               */
