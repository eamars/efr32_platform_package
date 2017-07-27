/*
 * \file sldp.h
 *
 * \brief Device preemption
 *
 * \details This file defines a device preemption interface.
 *
 *  Copyright (C) 2016 Silicon Labs, http://www.silabs.com
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef _DEVICE_PREEMPTION_H
#define _DEVICE_PREEMPTION_H

#include "slpal.h"
#include "em_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 ******************************* ERROR CODES  **********************************
 ******************************************************************************/
#define SLDP_ECODE_BUSY      (MBEDTLS_ERR_SLDP_BASE | 0x1)

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/

/** Device preemption context structure. */
typedef struct
{
  void               *sldpDevice;             /* Device context ref of type
                                               SLDP_DeviceContext_t. */
  void               *pDeviceContext;       /* Hardware context of device that
                                               needs to be saved when preempted
                                               and restored when resumed. */
  
  int                 lockWaitTicks;        /* Ticks to wait for ownership */
  unsigned long       priority;             /* Priority of context */
  
  /* Double linked-list for device preemption. */
  void*               pContextPreempted;    /* Context preempted by _this_
                                               context. */
  void*               pContextPreemptor;    /* Context of preemptor of _this_ 
                                               context. */
  
#if defined( SLDP_DEVICE_YIELD_WHEN_NOT_OWNER )
  bool                waitOwnership;       /**< Flag indicating that _this_
                                              context is waiting for ownership.
                                           */
  SLPAL_Completion_t  ownership;           /**< Completion object signaled by
                                              owner when ownership is returned
                                              to _this_ context. */
#endif
} SLDP_PreemptionContext_t;

/** Device Preemption device structure. */
typedef struct
{
  void                     *pDeviceHandle; /* User Device handle */
  
  SLDP_PreemptionContext_t *pOwner;        /* Pointer to current owner
                                              context of the device which
                                              serves as the anchor to a
                                              double linked list of all
                                              current "active" contexts.
                                              The Arbitrate function adds a
                                              new owner if ownership is won,
                                              and Release removes a context
                                              that is done (usually owner). */

  /* Mutex used to protect the data associated with the device
     when running in critical sections. */
  SLPAL_Mutex_t                lock;
  
  int(*enableDevice) (void* pDeviceHandle);      /* Pointer to function that
                                                    enables device. */
  int(*disableDevice)(void* pDeviceHandle);      /* Pointer to function that
                                                    disables device. */
  void(*storeContext)(void *dev, void *devCtx);  /* Pointer to function
                                                    that stores the context
                                                    of the device. */
  void(*loadContext) (void *dev, void *devCtx);  /* Pointer to function
                                                    that loads the context
                                                    of the device. */
#if defined( SLDP_ISR_PREEMPTION )
  bool            isrPreemption; /**< Flag that indicates whether ISR
                                    preemption is enabled for this device. */
  CORE_nvicMask_t nvicMask;      /**< NVIC mask of interrupts whose ISRs are
                                    allowed to preempt users of this device. */
  CORE_nvicMask_t nvicState;     /**< State of NVIC registers. Used to store
                                    NVIC state while interrupts are disabled.*/
#endif

} SLDP_DeviceContext_t;

void SLDP_DeviceInit( SLDP_DeviceContext_t *sldpDevice );
void SLDP_DeviceFree( SLDP_DeviceContext_t *sldpDevice );

void SLDP_DeviceHandleSet     ( SLDP_DeviceContext_t *sldpDevice,
                                void *pDeviceHandle);
void SLDP_DeviceEnableFuncSet ( SLDP_DeviceContext_t *sldpDevice,
                                int(*func)(void* pDeviceHandle) );
void SLDP_DeviceDisableFuncSet( SLDP_DeviceContext_t *sldpDevice,
                                int(*func)(void* pDeviceHandle) );
  
void SLDP_DeviceContextStoreFuncSet ( SLDP_DeviceContext_t *sldpDevice,
                                      void(*func)(void* pDeviceHandle,
                                                  void* devCtx) );
void SLDP_DeviceContextLoadFuncSet  ( SLDP_DeviceContext_t *sldpDevice,
                                      void(*func)(void* pDeviceHandle,
                                                  void* devCtx) );
#if defined( SLDP_ISR_PREEMPTION )
__STATIC_INLINE
void SLDP_DeviceIsrPreemptionEnable( SLDP_DeviceContext_t *sldpDevice,
                                     CORE_nvicMask_t      *nvicMask )
{
  sldpDevice->isrPreemption = true;
  sldpDevice->nvicMask      = *nvicMask;
}
#endif
  
__STATIC_INLINE
void SLDP_ContextInit( SLDP_PreemptionContext_t *ctx)
{
  ctx->priority      = 0;
  ctx->lockWaitTicks = 0;
#if defined( SLDP_DEVICE_YIELD_WHEN_NOT_OWNER )
  ctx->waitOwnership = false;
#endif
}

__STATIC_INLINE void SLDP_ContextDeviceSet
(
 SLDP_PreemptionContext_t *ctx,
 SLDP_DeviceContext_t     *sldpDevice
 )
{
  ctx->sldpDevice = sldpDevice;
}

__STATIC_INLINE void SLDP_ContextDeviceSpecificSet
(
 SLDP_PreemptionContext_t *ctx,
 void                     *pDeviceContext
 )
{
  ctx->pDeviceContext = pDeviceContext;
}

__STATIC_INLINE
void SLDP_ContextPrioritySet( SLDP_PreemptionContext_t *ctx,
                              unsigned int priority )
{
  ctx->priority = priority;
}

__STATIC_INLINE
void SLDP_ContextLockWaitTicksSet( SLDP_PreemptionContext_t *ctx,
                                   unsigned int lockWaitTicks )
{
  ctx->lockWaitTicks = lockWaitTicks;
}

int SLDP_Arbitrate    ( SLDP_PreemptionContext_t *ctx );
int SLDP_Release      ( SLDP_PreemptionContext_t *ctx );

int SLDP_CriticalEnter( SLDP_PreemptionContext_t *ctx );
int SLDP_CriticalExit ( SLDP_PreemptionContext_t *ctx );
  
#ifdef __cplusplus
}
#endif

#endif /* #ifndef _DEVICE_PREEMPTION_H */
