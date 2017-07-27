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
*                                    KERNEL TASK MANAGEMENT EXAMPLE
*
* File : ex_kernel_task_management.c
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
#include  "ex_kernel_task_management.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                  EXAMPLE CONFIGURATION REQUIREMENTS
*********************************************************************************************************
*********************************************************************************************************
*/

#if ((OS_CFG_TASK_DEL_EN       != DEF_ENABLED) || \
	 (OS_CFG_TASK_SUSPEND_EN   != DEF_ENABLED) || \
	 (OS_CFG_TIME_DLY_HMSM_EN  != DEF_ENABLED) || \
	 (OS_CFG_TASK_REG_TBL_SIZE  < 1u))
#error "This example requires OS_CFG_TASK_DEL_EN, OS_CFG_TASK_SUSPEND_EN and OS_CFG_TIME_DLY_HMSM_EN to be set to DEF_ENABLED. OS_CFG_TASK_REG_TBL_SIZE needs to be greater than 0u"
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

#define  EX_KERNEL_TASK_MANAGEMENT_TASK_STK_SIZE         128u
#define  EX_KERNEL_TASK_MANAGEMENT_TASK_PRIO              23u

#define  EX_KERNEL_MANIPULATION_TASK_STK_SIZE            128u
#define  EX_KERNEL_MANIPULATION_TASK_PRIO                 22u


/*
*********************************************************************************************************
*                                         EXAMPLE TASK REGISTER
*********************************************************************************************************
*/

#define  EX_KERNEL_TASK_MANAGEMENT_REG_VALUE              42u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Task Management Task Data.                           */
static  CPU_STK  Ex_KernelTaskManagementTaskStk[EX_KERNEL_TASK_MANAGEMENT_TASK_STK_SIZE];
static  OS_TCB   Ex_KernelTaskManagementTaskTCB;

                                                                /* Manipulation Task Data.                              */
static  CPU_STK  Ex_KernelManipulationTaskStk[EX_KERNEL_MANIPULATION_TASK_STK_SIZE];
static  OS_TCB   Ex_KernelManipulationTaskTCB;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_KernelTaskManagementTask(void  *p_arg);
static  void  Ex_KernelManipulationTask  (void  *p_arg);


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
*                                       Ex_KernelTaskManagement()
*
* Description : Illustrates the usage of task functions.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

void  Ex_KernelTaskManagement (void)
{
    RTOS_ERR  err;


    OSTaskCreate(&Ex_KernelTaskManagementTaskTCB,               /* Create Task Management Task.                         */
                 "Ex Kernel Task Management Task",              /*   Name of the task                                   */
                  Ex_KernelTaskManagementTask,                  /*   Pointer to the task implementation                 */
                  DEF_NULL,                                     /*   Task argument                                      */
                  EX_KERNEL_TASK_MANAGEMENT_TASK_PRIO,          /*   Task priority                                      */
                 &Ex_KernelTaskManagementTaskStk[0],            /*   Pointer to the task stack                          */
                                                                /*   Stack limit                                        */
                 (EX_KERNEL_TASK_MANAGEMENT_TASK_STK_SIZE / 10u),
                  EX_KERNEL_TASK_MANAGEMENT_TASK_STK_SIZE,      /*   Stack size                                         */
                  0,                                            /*   The maximum number of messages in the task queue   */
                  0,                                            /*   Time slice in tick for round-robin                 */
                  DEF_NULL,                                     /*   Pointer to user-supplied external TCB data         */
                 (OS_OPT_TASK_STK_CLR),                         /*   Contains additional information about the task     */
                 &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    OSTaskCreate(&Ex_KernelManipulationTaskTCB,                 /* Create Manipulation Task.                            */
                 "Ex Kernel Manipulation Task",                 /*   Name of the task                                   */
                  Ex_KernelManipulationTask,                    /*   Pointer to the task implementation                 */
                  DEF_NULL,                                     /*   Task argument                                      */
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
*                                     Ex_KernelTaskManagementTask()
*
* Description : This task simply delays itsef.
*
* Argument(s) : p_arg
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  Ex_KernelTaskManagementTask (void  *p_arg)
{
    RTOS_ERR  err;


                                                                /* Prevent compiler warning.                            */
    PP_UNUSED_PARAM(p_arg);

    while(DEF_ON){

                                                                /* Delay Task execution for                             */
        OSTimeDly( 500,                                         /*   500 OS Ticks                                       */
                   OS_OPT_TIME_DLY,                             /*   from now.                                          */
                  &err);
        APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

        EX_TRACE("Management Task is alive\r\n");
    }
}


/*
*********************************************************************************************************
*                                      Ex_KernelManipulationTask()
*
* Description : This task illustrates the task management functions.
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
    RTOS_ERR   err;
    OS_REG     get_register;
    OS_REG_ID  register_id;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */

                                                                /* Delay Task execution for                             */
    OSTimeDly( 1500,                                            /*   1500 OS Ticks                                      */
               OS_OPT_TIME_DLY,                                 /*   from now.                                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Suspend Ex_KernelTaskManagementTask for 5 seconds\r\n");
    OSTaskSuspend(&Ex_KernelTaskManagementTaskTCB,              /* Suspend Ex_KernelTaskManagementTask                  */
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    register_id = OSTaskRegGetID(&err);                         /* Get a new register ID                                */
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("Get a new register ID: %u\r\n", register_id);

    EX_TRACE("Set register value for register %u\r\n", register_id);
    OSTaskRegSet(&Ex_KernelTaskManagementTaskTCB,               /* Set register value                                   */
                 register_id,                                   /*   Register ID                                        */
                 EX_KERNEL_TASK_MANAGEMENT_REG_VALUE,           /*   Register value                                     */
                &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    get_register = OSTaskRegGet(&Ex_KernelTaskManagementTaskTCB,/* Get register value                                   */
                                 register_id,                   /*   Register ID                                        */
                                &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    EX_TRACE("Get value for register %u: %u\r\n", register_id, get_register);


                                                                /* Delay Task execution for                             */
    OSTimeDly( 5000,                                            /*   5000 OS Ticks                                      */
               OS_OPT_TIME_DLY,                                 /*   from now.                                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Resume Ex_KernelTaskManagementTask\r\n");
    OSTaskResume(&Ex_KernelTaskManagementTaskTCB,               /* Resume Ex_KernelTaskManagementTask                   */
                 &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    OSTimeDlyHMSM( 0,                                           /* Delay task for 1.5 seconds                           */
                   0,
                   1,
                   500,
                   OS_OPT_TIME_HMSM_STRICT,
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    EX_TRACE("Delete Ex_KernelTaskManagementTask\r\n");
    OSTaskDel(&Ex_KernelTaskManagementTaskTCB,                  /* Delete Task Management Task                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    OSTaskDel( DEF_NULL,                                        /* Delete self                                          */
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}
