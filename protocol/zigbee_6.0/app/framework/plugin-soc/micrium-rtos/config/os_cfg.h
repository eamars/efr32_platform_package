/*
 *********************************************************************************************************
 *
 *                                         KERNEL CONFIGURATION
 *
 * Filename      : os_cfg.h
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                               MODULE
 *********************************************************************************************************
 *********************************************************************************************************
 */

#ifndef  _OS_CFG_H_
#define  _OS_CFG_H_

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#include  <rtos/common/include/lib_def.h>

#define  OS_CFG_APP_HOOKS_EN                                DEF_ENABLED
#define  OS_CFG_COMPAT_INIT_EN                              DEF_ENABLED
#define  OS_CFG_DBG_EN                                      DEF_ENABLED
#define  OS_CFG_DYN_TICK_EN                                 DEF_DISABLED
#define  OS_CFG_TS_EN                                       DEF_ENABLED

#define  OS_CFG_PRIO_MAX                                    16u

#define  OS_CFG_SCHED_LOCK_TIME_MEAS_EN                     DEF_ENABLED
#define  OS_CFG_SCHED_ROUND_ROBIN_EN                        DEF_DISABLED

#define  OS_CFG_STK_SIZE_MIN                                64u

#define  OS_CFG_FLAG_EN                                     DEF_ENABLED
#define  OS_CFG_FLAG_DEL_EN                                 DEF_DISABLED
#define  OS_CFG_FLAG_MODE_CLR_EN                            DEF_DISABLED
#define  OS_CFG_FLAG_PEND_ABORT_EN                          DEF_DISABLED

#define  OS_CFG_MEM_EN                                      DEF_DISABLED

#define  OS_CFG_MUTEX_EN                                    DEF_ENABLED
#define  OS_CFG_MUTEX_DEL_EN                                DEF_ENABLED
#define  OS_CFG_MUTEX_PEND_ABORT_EN                         DEF_DISABLED

#define  OS_CFG_Q_EN                                        DEF_ENABLED
#define  OS_CFG_Q_DEL_EN                                    DEF_DISABLED
#define  OS_CFG_Q_FLUSH_EN                                  DEF_DISABLED
#define  OS_CFG_Q_PEND_ABORT_EN                             DEF_DISABLED

#define  OS_CFG_SEM_EN                                      DEF_ENABLED
#define  OS_CFG_SEM_DEL_EN                                  DEF_ENABLED
#define  OS_CFG_SEM_PEND_ABORT_EN                           DEF_DISABLED
#define  OS_CFG_SEM_SET_EN                                  DEF_DISABLED

#define  OS_CFG_MON_EN                                      DEF_DISABLED
#define  OS_CFG_MON_DEL_EN                                  DEF_DISABLED

#define  OS_CFG_STAT_TASK_EN                                DEF_ENABLED
#define  OS_CFG_STAT_TASK_STK_CHK_EN                        DEF_ENABLED
#define  OS_CFG_TASK_DEL_EN                                 DEF_ENABLED
#define  OS_CFG_TASK_IDLE_EN                                DEF_ENABLED
#define  OS_CFG_TASK_PROFILE_EN                             DEF_ENABLED
#define  OS_CFG_TASK_Q_EN                                   DEF_DISABLED
#define  OS_CFG_TASK_Q_PEND_ABORT_EN                        DEF_DISABLED
#define  OS_CFG_TASK_REG_TBL_SIZE                           3u
#define  OS_CFG_TASK_STK_REDZONE_EN                         DEF_ENABLED
#define  OS_CFG_TASK_STK_REDZONE_DEPTH                      8u
#define  OS_CFG_TASK_SEM_PEND_ABORT_EN                      DEF_DISABLED
#define  OS_CFG_TASK_SUSPEND_EN                             DEF_DISABLED
#define  OS_CFG_TASK_TICK_EN                                DEF_ENABLED

#define  OS_CFG_TLS_TBL_SIZE                                0u

#define  OS_CFG_TIME_DLY_HMSM_EN                            DEF_ENABLED
#define  OS_CFG_TIME_DLY_RESUME_EN                          DEF_ENABLED

#define  OS_CFG_TMR_EN                                      DEF_ENABLED
#define  OS_CFG_TMR_DEL_EN                                  DEF_ENABLED

#define  OS_CFG_TRACE_EN                                    DEF_DISABLED

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             MODULE END
 *********************************************************************************************************
 *********************************************************************************************************
 */

#endif /* End of os_cfg.h module include.                      */
