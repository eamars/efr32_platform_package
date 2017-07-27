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
*                                     KERNEL TIME MANAGEMENT EXAMPLE
*
* File : ex_kernel_time_management.c
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
#include  "ex_kernel_time_management.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                  EXAMPLE CONFIGURATION REQUIREMENTS
*********************************************************************************************************
*********************************************************************************************************
*/

#if ((OS_CFG_TASK_DEL_EN        != DEF_ENABLED) || \
     (OS_CFG_TIME_DLY_HMSM_EN   != DEF_ENABLED) || \
     (OS_CFG_TIME_DLY_RESUME_EN != DEF_ENABLED))
#error "This example requires OS_CFG_TASK_DEL_EN, OS_CFG_TIME_DLY_HMSM_EN and OS_CFG_TIME_DLY_RESUME_EN to be set to DEF_ENABLED"
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

                                                                /* Pend Task Configuration.                             */
#define  EX_KERNEL_TIME_MANAGEMENT_TASK_STK_SIZE         128u
#define  EX_KERNEL_TIME_MANAGEMENT_TASK_PRIO              22u

#define  EX_KERNEL_RESUME_TASK_STK_SIZE                  128u
#define  EX_KERNEL_RESUME_TASK_PRIO                       23u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Time Management Task Data.                           */
static  CPU_STK  Ex_KernelTimeManagementTaskStk[EX_KERNEL_TIME_MANAGEMENT_TASK_STK_SIZE];
static  OS_TCB   Ex_KernelTimeManagementTaskTCB;

                                                                /* Resume Task Data.                                    */
static  CPU_STK  Ex_KernelResumeTaskStk[EX_KERNEL_RESUME_TASK_STK_SIZE];
static  OS_TCB   Ex_KernelResumeTaskTCB;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_KernelTimeManagementTask(void  *p_arg);
static  void  Ex_KernelResumeTask        (void  *p_arg);


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
*                                       Ex_KernelTimeManagement()
*
* Description : Illustrates the usage of kernel time functions.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

void  Ex_KernelTimeManagement (void)
{
    RTOS_ERR  err;


    OSTaskCreate(&Ex_KernelTimeManagementTaskTCB,               /* Create Time Management Task.                         */
                 "Ex Kernel Time Management Task",
                  Ex_KernelTimeManagementTask,
                  0,
                  EX_KERNEL_TIME_MANAGEMENT_TASK_PRIO,
                 &Ex_KernelTimeManagementTaskStk[0],
                 (EX_KERNEL_TIME_MANAGEMENT_TASK_STK_SIZE / 10u),
                  EX_KERNEL_TIME_MANAGEMENT_TASK_STK_SIZE,
                  0,
                  0,
                  0,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* Create Resume Task.                                  */
    OSTaskCreate(&Ex_KernelResumeTaskTCB,
                 "Ex Kernel Resume Task",
                  Ex_KernelResumeTask,
                  0,
                  EX_KERNEL_RESUME_TASK_PRIO,
                 &Ex_KernelResumeTaskStk[0],
                 (EX_KERNEL_RESUME_TASK_STK_SIZE / 10u),
                  EX_KERNEL_RESUME_TASK_STK_SIZE,
                  0,
                  0,
                  0,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

                                                                /* Delay Task execution for                             */
    OSTimeDly( 3000,                                            /*   3000 OS Ticks                                      */
               OS_OPT_TIME_DLY,                                 /*   from now.                                          */
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
*                                     Ex_KernelTimeManagementTask()
*
* Description : This task manipulate time function.
*
* Argument(s) : p_arg
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_KernelTimeManagementTask (void  *p_arg)
{
    RTOS_ERR  err;
    OS_TICK   get_time;
    OS_TICK   set_time;


                                                                /* Prevent compiler warning.                            */
    PP_UNUSED_PARAM(p_arg);


    EX_TRACE("Start 1000 OS Ticks delay\r\n");
                                                                /* Delay Task execution for                             */
    OSTimeDly( 1000,                                            /*   1000 OS Ticks                                      */
               OS_OPT_TIME_DLY,                                 /*   from now.                                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("Returned from 1000 OS Ticks delay, start 20 seconds delay\r\n");


                                                                /* Delay Task execution for                             */
    OSTimeDlyHMSM( 0,                                           /*   0 hours                                            */
                   0,                                           /*   0 minutes                                          */
                   20,                                          /*   20 seconds                                         */
                   0,                                           /*   0 milliseconds                                     */
                   OS_OPT_TIME_HMSM_STRICT,                     /*   Allow only valid H:M:S:m delays                    */
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("Returned early from 20 seconds delay\r\n");


    get_time = OSTimeGet(&err);                                 /* Get tick value                                       */
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("OS Time = %u\r\n", get_time);


    set_time = 1000;
    OSTimeSet( set_time,                                        /* Set a desired tick value.                            */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}


/*
*********************************************************************************************************
*                                        Ex_KernelResumeTask()
*
* Description : This task resumes Ex_KernelTimeManagementTask.
*
* Argument(s) : p_arg
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_KernelResumeTask (void  *p_arg)
{
    RTOS_ERR  err;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */


    OSTimeDlyHMSM( 0,                                           /* Delay task for 2 seconds                             */
                   0,
                   2,
                   0,
                   OS_OPT_TIME_HMSM_STRICT,
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Resume Ex_KernelTimeManagementTask\r\n");
    OSTimeDlyResume(&Ex_KernelTimeManagementTaskTCB,            /* Resume Ex_KernelTimeManagementTask from its delay    */
                    &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Delete Ex_KernelResourceManagementTask task\r\n");
    OSTaskDel(&Ex_KernelTimeManagementTaskTCB,                  /* Delete Time Management Task                      */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Delete current task\r\n");
    OSTaskDel( DEF_NULL,                                        /* Delete self                                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}
