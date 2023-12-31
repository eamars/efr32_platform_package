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
*                                       COMMON LIB MEM EXAMPLE
*
* File : ex_common_lib_mem_dyn_pool_get_free.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>
#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  EX_MEM_SEG_DATA_SIZE                            512u
#define  EX_LIB_MEM_DYN_POOL_GET_FREE_POOL_BLK_INIT        3u
#define  EX_LIB_MEM_DYN_POOL_GET_FREE_POOL_BLK_MAX         6u


/*
*********************************************************************************************************
*                                               LOGGING
*
* Note(s) : (1) This example outputs information to the console via the function printf() via a macro
*               called EX_TRACE(). This can be modified or disabled if printf() is not supported.
*********************************************************************************************************
*/

#ifndef  EX_TRACE
#include  <stdio.h>
#define  EX_TRACE(...)                                  printf(__VA_ARGS__)
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  MEM_SEG       ExMemSeg;
static  CPU_INT08U    ExMemSegData[EX_MEM_SEG_DATA_SIZE];       /* Could specify/use particular location for this data. */
static  MEM_DYN_POOL  ExMemPool;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                    Ex_CommonLibMemDynPoolGetFree()
*
* Description : Provide examples of Mem_SegCreate(), Mem_DynPoolCreate(), Mem_DynPoolBlkGet() and
*               Mem_DynPoolBlkFree() functions. This example show the basic manipulation of a pool.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  Ex_CommonLibMemDynPoolGetFree (void)
{
    RTOS_ERR     err;
    CPU_INT64U  *p_data_get_1;
    CPU_INT64U  *p_data_get_2;
    CPU_INT64U  *p_data_get_3;
    CPU_INT64U  *p_data_get_4;


                                                                /* Create regular, general-purpose, memory segment.     */
    Mem_SegCreate("Example memory segment",                     /* Name of the memory segment, for debug purposes.      */
                  &ExMemSeg,                                    /* Pointer to MEM_SEG structure.                        */
                  (CPU_ADDR)&(ExMemSegData[0u]),                /* Pointer to data start location.                      */
                   EX_MEM_SEG_DATA_SIZE,                        /* Segment size.                                        */
                   LIB_MEM_PADDING_ALIGN_NONE,                  /* No padding alignment required.                       */
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* Create a dynamic memory pool.                        */
                                                                /* Name of the block, for debugging purposes.           */
    Mem_DynPoolCreate("Example block from example memory segment",
                      &ExMemPool,                               /* Pointer to the pool data.                            */
                      &ExMemSeg,                                /* Pointer to MEM_SEG from which to allocate.           */
                      sizeof(CPU_INT64U),                       /* Size of memory block to allocate from pool.          */
                      sizeof(CPU_ALIGN),                        /* Alignment of memory blocks.                          */
                      EX_LIB_MEM_DYN_POOL_GET_FREE_POOL_BLK_INIT, /* Number of elements to be allocated from the pool.  */
                      EX_LIB_MEM_DYN_POOL_GET_FREE_POOL_BLK_MAX,/* Maximum elements to be allocated from the pool.      */
                      &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* Pointer to the pool data.                            */
    p_data_get_1 = (CPU_INT64U *)Mem_DynPoolBlkGet(&ExMemPool, &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("p_data_get_1 address: %p\n", p_data_get_1);

    *p_data_get_1 = 0xAAAAAAAAAAAAAAAA;

                                                                /* Pointer to the pool data.                            */
    p_data_get_2 = (CPU_INT64U *)Mem_DynPoolBlkGet(&ExMemPool, &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("p_data_get_2 address: %p\n", p_data_get_2);

    *p_data_get_2 = 0xBBBBBBBBBBBBBBBB;

                                                                /* Pointer to the pool data.                            */
    p_data_get_3 = (CPU_INT64U *)Mem_DynPoolBlkGet(&ExMemPool, &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("p_data_get_3 address: %p\n", p_data_get_3);

    *p_data_get_3 = 0xCCCCCCCCCCCCCCCC;

                                                                /* Pointer to the pool data.                            */
    p_data_get_4 = (CPU_INT64U *)Mem_DynPoolBlkGet(&ExMemPool, &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("p_data_get_4 address: %p\n", p_data_get_4);

    *p_data_get_4 = 0xDDDDDDDDDDDDDDDD;

    Mem_DynPoolBlkFree(&ExMemPool,                              /* Pointer to the pool data.                            */
                (void *)p_data_get_4,                           /* Pointer to first byte of memory block.               */
                       &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* Pointer to the pool data.                            */
    p_data_get_4 = (CPU_INT64U *)Mem_DynPoolBlkGet(&ExMemPool, &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("p_data_get_4 address: %p\n", p_data_get_4);

    *p_data_get_4 = 0xABCDEFFFFFFFFFFF;
}
