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
*                                  KERNEL RESOURCE MANAGEMENT EXAMPLE
*
* File : ex_kernel_resource_management.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/kernel/include/os.h>
#include  <rtos/common/include/common.h>
#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/toolchains.h>
#include  "ex_kernel_resource_management.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                  EXAMPLE CONFIGURATION REQUIREMENTS
*********************************************************************************************************
*********************************************************************************************************
*/

#if ((OS_CFG_TASK_DEL_EN       != DEF_ENABLED) || \
     (OS_CFG_SEM_DEL_EN        != DEF_ENABLED) || \
     (OS_CFG_SEM_PEND_ABORT_EN != DEF_ENABLED) || \
     (OS_CFG_TIME_DLY_HMSM_EN  != DEF_ENABLED))
#error "This example requires OS_CFG_TASK_DEL_EN, OS_CFG_SEM_DEL_EN, OS_CFG_SEM_PEND_ABORT_EN and OS_CFG_TIME_DLY_HMSM_EN to be set to DEF_ENABLED"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

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
#define  EX_TRACE(...)                                      printf(__VA_ARGS__)
#endif


/*
*********************************************************************************************************
*                                         TASK'S CONFIGURATION
*********************************************************************************************************
*/

#define  EX_KERNEL_RESOURCE_MANAGEMENT_TASK_STK_SIZE     128u
#define  EX_KERNEL_RESOURCE_MANAGEMENT_TASK_PRIO          22u

#define  EX_KERNEL_MANIPULATION_TASK_STK_SIZE            128u
#define  EX_KERNEL_MANIPULATION_TASK_PRIO                 23u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  EX_KERNEL_POST_MAX_LOOP                           3u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Resource Management Task Data.                       */
static  CPU_STK  Ex_KernelResourceManagementTaskStk[EX_KERNEL_RESOURCE_MANAGEMENT_TASK_STK_SIZE];
static  OS_TCB   Ex_KernelResourceManagementTaskTCB;

                                                                /* Manipulation Task Data.                              */
static  CPU_STK  Ex_KernelManipulationTaskStk[EX_KERNEL_MANIPULATION_TASK_STK_SIZE];
static  OS_TCB   Ex_KernelManipulationTaskTCB;

static  OS_SEM   ExKernelResourceManagement_SemExample;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_KernelResourceManagementTask(void  *p_arg);
static  void  Ex_KernelManipulationTask      (void  *p_arg);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     Ex_KernelResourceManagement()
*
* Description : Illustrates the usage of kernel semaphore functions.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

void  Ex_KernelResourceManagement (void)
{
    RTOS_ERR  err;


    OSSemCreate(&ExKernelResourceManagement_SemExample,         /* Pointer to a semaphore                               */
                "Example semaphore",                            /*   Semaphore name, for debugging purpose              */
                 1,                                             /*   Initial value of the semaphore                     */
                &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    OSSemSet(&ExKernelResourceManagement_SemExample,            /* Pointer to a semaphore                               */
              0,                                                /*   Reset the semaphore                                */
             &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    OSTaskCreate(&Ex_KernelResourceManagementTaskTCB,           /* Create Resource Management Task                      */
                 "Ex Kernel Resource Management Task",          /*   Name of the task                                   */
                  Ex_KernelResourceManagementTask,              /*   Pointer to the task implementation                 */
                  DEF_NULL,                                     /*   Task argument                                      */
                  EX_KERNEL_RESOURCE_MANAGEMENT_TASK_PRIO,      /*   Task priority                                      */
                 &Ex_KernelResourceManagementTaskStk[0],        /*   Pointer to the task stack                          */
                                                                /*   Stack limit                                        */
                 (EX_KERNEL_RESOURCE_MANAGEMENT_TASK_STK_SIZE / 10u),
                  EX_KERNEL_RESOURCE_MANAGEMENT_TASK_STK_SIZE,  /*   Stack size                                         */
                  0,                                            /*   The maximum number of messages in the task queue   */
                  0,                                            /*   Time slice in tick for round-robin                 */
                  DEF_NULL,                                     /*   Pointer to user-supplied external TCB data         */
                 (OS_OPT_TASK_STK_CLR),                         /*   Contains additional information about the task     */
                 &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    OSTaskCreate(&Ex_KernelManipulationTaskTCB,                 /* Create Manipulation Task                             */
                 "Ex Kernel Manipulation Task",                 /*   Name of the task                                   */
                  Ex_KernelManipulationTask,                    /*   Pointer to the task implementation                 */
                  0,                                            /*   Task argument                                      */
                  EX_KERNEL_MANIPULATION_TASK_PRIO,             /*   Task priority                                      */
                 &Ex_KernelManipulationTaskStk[0],              /*   Pointer to the task stack                          */
                                                                /*   Stack limit                                        */
                 (EX_KERNEL_MANIPULATION_TASK_STK_SIZE / 10u),
                  EX_KERNEL_MANIPULATION_TASK_STK_SIZE,         /*   Stack size                                         */
                  0,                                            /*   The maximum number of messages in the task queue   */
                  0,                                            /*   Time slice in tick for round-robin                 */
                  DEF_NULL,                                     /*   Pointer to user-supplied external TCB data         */
                 (OS_OPT_TASK_STK_CLR),                         /*   Contains additional information about the task     */
                 &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* Delay Task execution for                             */
    OSTimeDly( 3000,                                            /*   3000 OS Ticks                                      */
               OS_OPT_TIME_DLY,                                 /*   from now.                                          */
              &err);
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
*                                   Ex_KernelResourceManagementTask()
*
* Description : This task pends on the semaphore.
*
* Argument(s) : p_arg
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_KernelResourceManagementTask (void  *p_arg)
{
    RTOS_ERR  err;
    CPU_TS    timestamp;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */

    while (DEF_ON) {
        EX_TRACE("Pend on semaphore\r\n");
        OSSemPend(&ExKernelResourceManagement_SemExample,       /* Pointer to a semaphore                               */
                   0,                                           /*   Set timeout, 0 to wait forever on the semaphore    */
                   OS_OPT_PEND_BLOCKING,                        /*   Pend semaphore in blocking mode                    */
                  &timestamp,                                   /*   Timestamp of when semaphore was released           */
                  &err);
        if (err.Code == RTOS_ERR_ABORT) {
            EX_TRACE("Pend on semaphore was aborted\r\n");
        } else {
                                                                /* Handle error(s).                                     */
        }
    }
}


/*
*********************************************************************************************************
*                                      Ex_KernelManipulationTask()
*
* Description : This task illustrates task control using a semaphore.
*
* Argument(s) : p_arg
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_KernelManipulationTask (void  *p_arg)
{
    RTOS_ERR    err;
    CPU_INT08U  post_counter;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */

    post_counter = 0;
    while (DEF_ON) {

        if(post_counter < EX_KERNEL_POST_MAX_LOOP) {

            EX_TRACE("Post the semaphore\r\n");
            OSSemPost(&ExKernelResourceManagement_SemExample,   /* Pointer to a semaphore                               */
                        OS_OPT_POST_ALL,                        /*   Post and ready all tasks waiting on the semaphore  */
                       &err);

            post_counter++;

        } else {

            EX_TRACE("Pend abort the semaphore\r\n");
            OSSemPendAbort(&ExKernelResourceManagement_SemExample, /* Pointer to a semaphore                            */
                           OS_OPT_PEND_ABORT_ALL,                  /*   Abort the pend for all tasks                    */
                           &err);
            break;
        }
    }


    OSTimeDlyHMSM( 0,                                           /* Delay task for 1 second                              */
                   0,
                   1,
                   0,
                   OS_OPT_TIME_HMSM_STRICT,
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Delete Resource Management Task\r\n");
    OSTaskDel(&Ex_KernelResourceManagementTaskTCB,              /* Delete Resource Management Task                      */
               &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Delete the semaphore\r\n");
    OSSemDel(&ExKernelResourceManagement_SemExample,            /* Pointer to a semaphore                               */
              OS_OPT_DEL_ALWAYS,                                /*   Delete semaphore even if a task is waiting         */
             &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Delete current task\r\n");
    OSTaskDel( DEF_NULL,                                        /* Delete self                                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}

