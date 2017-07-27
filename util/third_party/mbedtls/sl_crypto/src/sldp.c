/*
 *  Device Preemption
 *
 *  Copyright (C) 2016-2017, Silicon Labs, http://www.silabs.com
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

#include "sldp.h"
#include <string.h>

/*******************************************************************************
 ******************************   FUNCTIONS   **********************************
 ******************************************************************************/

#if defined( SLDP_ISR_PREEMPTION )

__STATIC_INLINE
void sldpDeviceNvicMaskDisable(SLDP_DeviceContext_t *sldpDevice)
{
  if (sldpDevice->isrPreemption)
  {
    SLPAL_irqState_t irqState;

    /* Enter OS dependent critical section in order to make sure reading and
       writing to NVIC ICER registers is atomic (not interrupted by
       ISRs or threads). */
  
    irqState = SLPAL_CriticalEnter();
    
    /* Read the state of NVIC ICER registers before disabling. */
    sldpDevice->nvicState = *(CORE_nvicMask_t*)&NVIC->ICER[0];
  
    /* Disable the interrupts whose ISRs are allowed to preempt
       users of this device. */
    *(CORE_nvicMask_t*)&NVIC->ICER[0] = sldpDevice->nvicMask;
    
    SLPAL_CriticalExit(irqState);
  }
}

__STATIC_INLINE
void sldpDeviceNvicMaskEnable(SLDP_DeviceContext_t *sldpDevice)
{
  if (sldpDevice->isrPreemption)
  {
    SLPAL_irqState_t irqState;
    CORE_nvicMask_t  enableMask;
    int              i;

    for (i=0; i<CORE_NVIC_REG_WORDS; i++)
    {
      enableMask.a[i] = 
        sldpDevice->nvicState.a[i] & sldpDevice->nvicMask.a[i];
    }

    /* Enter OS dependent critical section in order to make sure writing to
       NVIC ISER registers is atomic. */
  
    irqState = SLPAL_CriticalEnter();
    
    /* Enable interrupts whose ISRs are allowed to preempt. */
    *(CORE_nvicMask_t*)&NVIC->ISER[0] = enableMask;

    SLPAL_CriticalExit(irqState);
  }
}

#endif  /* #if defined( SLDP_ISR_PREEMPTION ) */

#if defined( SLDP_FREERTOS ) || defined( SLDP_UCOS3 )

__STATIC_INLINE int sldpWaitForOwnership(SLDP_PreemptionContext_t* ctx)
{
  SLDP_DeviceContext_t     *sldpDevice = ctx->sldpDevice;
  SLPAL_Mutex_t            *lock       = &sldpDevice->lock;
  int                       ret;

  ret = SLPAL_TakeMutex( lock, SLPAL_WAIT_FOREVER );
  EFM_ASSERT(ret == 0);
  
  /* Loop until the context becomes owner of the device. */
  while (ctx != sldpDevice->pOwner)
  {
#if defined( SLDP_DEVICE_YIELD_WHEN_NOT_OWNER )
    ctx->waitOwnership = true;
    EFM_ASSERT(SLPAL_InitCompletion(&ctx->ownership) == 0);
#endif
    
    /* The context does not own the device. Therefore release the device lock
       and wait for the ownership to be given back (de-preempted).*/
    ret = SLPAL_GiveMutex( lock );
    EFM_ASSERT(ret == 0);
    
#if defined( SLDP_DEVICE_YIELD_WHEN_NOT_OWNER )
    /* Wait for preemptor to signal the return of ownership using the
       ownership completion object. */
    ret = SLPAL_WaitForCompletion(&ctx->ownership, SLPAL_WAIT_FOREVER);
    SLPAL_FreeCompletion(&ctx->ownership);
#endif

    /* Take the device lock and recheck if the ownership has been returned to
       this context (de-preempted). */
    ret = SLPAL_TakeMutex( lock, SLPAL_WAIT_FOREVER );
    EFM_ASSERT(ret == 0);
  }
  return 0;
}

#else  /* #if defined( SLDP_FREERTOS )  || defined( SLDP_UCOS3 ) */

#if defined (SLDP_CRITICAL_ENTER_FAIL_IF_NOT_OWNER)

__STATIC_INLINE int sldpWaitForOwnership(SLDP_PreemptionContext_t* ctx)
{
  SLDP_DeviceContext_t  *sldpDevice = (SLDP_DeviceContext_t*) ctx->sldpDevice;
  int                   ret;
  
  ret = SLPAL_TakeMutex( &sldpDevice->lock, SLPAL_WAIT_FOREVER );
  EFM_ASSERT(ret == 0);
  
  if (ctx != sldpDevice->pOwner)
  {
    return SLDP_ECODE_BUSY;
  }
  else
  {
    return 0;
  }
}

#else /* #if defined (SLDP_CRITICAL_ENTER_FAIL_IF_NOT_OWNER) */

__STATIC_INLINE int sldpWaitForOwnership(SLDP_PreemptionContext_t* ctx)
{
  SLDP_DeviceContext_t   *sldpDevice = ctx->sldpDevice;
  SLPAL_Mutex_t          *lock       = &sldpDevice->lock;
  /* Take the device lock. */
  return SLPAL_TakeMutex( lock, SLPAL_WAIT_FOREVER );
}

#endif /* #if defined (SLDP_CRITICAL_ENTER_FAIL_IF_NOT_OWNER) */

#endif /* #if defined( SLDP_FREERTOS )  || defined( SLDP_UCOS3 ) */

void SLDP_DeviceInit( SLDP_DeviceContext_t *sldpDevice )
{
  int status;
  memset(sldpDevice, 0, sizeof( SLDP_DeviceContext_t ) );

  status = SLPAL_InitMutex(&sldpDevice->lock);
  EFM_ASSERT(status == 0);
}

void SLDP_DeviceFree( SLDP_DeviceContext_t *sldpDevice )
{
  int status;
  status = SLPAL_FreeMutex(&sldpDevice->lock);
  EFM_ASSERT(status == 0);
  memset(sldpDevice, 0, sizeof( SLDP_DeviceContext_t ) );
}

void SLDP_DeviceHandleSet( SLDP_DeviceContext_t *sldpDevice,
                           void *pDeviceHandle)
{
  sldpDevice->pDeviceHandle = pDeviceHandle;
}

void SLDP_DeviceEnableFuncSet( SLDP_DeviceContext_t *sldpDevice,
                               int(*func)(void* pDeviceHandle) )
{
  sldpDevice->enableDevice = func;
}

void SLDP_DeviceDisableFuncSet( SLDP_DeviceContext_t *sldpDevice,
                                int(*func)(void* pDeviceHandle) )
{
  sldpDevice->disableDevice = func;
}
  
void SLDP_DeviceContextStoreFuncSet( SLDP_DeviceContext_t *sldpDevice,
                                     void(*func)(void* pDeviceHandle,
                                                 void* devCtx) )
{
  sldpDevice->storeContext = func;
}
  
void SLDP_DeviceContextLoadFuncSet( SLDP_DeviceContext_t *sldpDevice,
                                    void(*func)(void* pDeviceHandle,
                                                void* devCtx) )
{
  sldpDevice->loadContext = func;
}

/***************************************************************************//**
 * @brief
 *  Arbitrate for exclusive access to the device.
 *
 * @details
 *  This function will arbitrate for the ownership of the device. If there is
 *  a current owner and the caller wins ownership, this function will preempt
 *  the current owner by storing the device context with a call to the
 *  storeContext function. When the new owner (caller) is ready to release
 *  the device, the caller should call SLDP_Release in
 *  order to restore the context of the preemptee, in order to
 *  continue operation.
 *
 * @param ctx
 *  Pointer to a device preemption context.
 *
 * @return
 *  0 if success. -1 if device ownership was not won.
 */
int SLDP_Arbitrate( SLDP_PreemptionContext_t *ctx )
{
  int                   ret;
  SLDP_DeviceContext_t *sldpDevice = (SLDP_DeviceContext_t*) ctx->sldpDevice;
  SLPAL_Mutex_t        *lock = &sldpDevice->lock;
  SLDP_PreemptionContext_t *pOwner;

#if !defined( SLDP_CONTEXT_PRIORITY_SET )
  ctx->priority = SLPAL_ThreadPriorityGet();
#endif
  
  if (false == RUNNING_AT_INTERRUPT_LEVEL)
  {
    ret = SLPAL_TakeMutex( lock, ctx->lockWaitTicks );
  
    if ( ret == SLPAL_ERROR_TIMEOUT )
    {
      return SLDP_ECODE_BUSY;
    }
  }
  
#if defined( SLDP_ISR_PREEMPTION )
  /* Disable the interrupts whose ISRs are allowed to preempt. */
  sldpDeviceNvicMaskDisable( sldpDevice );
#endif
  
  pOwner = sldpDevice->pOwner;
  
  /* Check if someone is currently owning the device. */
  if (pOwner)
  {
    if ( ctx->priority <= pOwner->priority )
    {
      /* Arbitration lost, return error. */
      ret = SLDP_ECODE_BUSY;
    }
    else
    {
      /* Arbitration won, preempt current owner by storing it's context. */
      pOwner->pContextPreemptor = ctx;
      
      EFM_ASSERT(sldpDevice->storeContext!=NULL);
      sldpDevice->storeContext(sldpDevice->pDeviceHandle,
                               pOwner->pDeviceContext);
      
      ctx->pContextPreempted = pOwner; /* Store preempted context */
      sldpDevice->pOwner  = ctx;    /* Set new owner to this context in
                                     sldpDevice descriptor */
      ctx->pContextPreemptor = 0;
    
      ret = 0;
    }
  }
  else
  {
    /* No current owner, just enable device, and . */
    EFM_ASSERT(sldpDevice->enableDevice!=NULL);
    sldpDevice->enableDevice(sldpDevice->pDeviceHandle);
    
    sldpDevice->pOwner      = ctx;  /* Set owner to this context in sldpDevice
                                       descriptor */
    ctx->pContextPreempted = 0;
    ctx->pContextPreemptor = 0;
    
    ret = 0;
  }
  
#if defined( SLDP_ISR_PREEMPTION )
  /* Enable interrupts whose ISRs are allowed to preempt. */
  sldpDeviceNvicMaskEnable( sldpDevice );
#endif
  
  if (false == RUNNING_AT_INTERRUPT_LEVEL)
  {
    SLPAL_GiveMutex( lock );
  }
  
  return ret;
}

/***************************************************************************//**
 * @brief
 *  Release a context and possibly give ownership of device to preempted.
 *
 * @details
 *  This function removes a context from the device preemption context list
 *  (which includes current owner and preempted contexts).
 *  If the specified context is the owner of the devuce, this function
 *  will release the ownership of the device. If there is a preempted
 *  context in the context list, then the preempted context is restored
 *  by updating the hardware state of the device, and the preempted
 *  context becomes owner again.
 *
 * @param ctx
 *  Pointer to a device preemption context.
 *
 * @return
 *  Allways 0 for success.
 */
int SLDP_Release( SLDP_PreemptionContext_t *ctx )
{
  SLDP_PreemptionContext_t *preempted;
  SLDP_PreemptionContext_t *preemptor;
  SLDP_DeviceContext_t *sldpDevice = (SLDP_DeviceContext_t*) ctx->sldpDevice;
  SLPAL_Mutex_t        *lock = &sldpDevice->lock;
  int                   ret;
  
  if (false == RUNNING_AT_INTERRUPT_LEVEL)
  {
    ret = SLPAL_TakeMutex( lock, SLPAL_WAIT_FOREVER );
    EFM_ASSERT(ret == 0);
  }
  
#if defined( SLDP_ISR_PREEMPTION )
  /* Disable the interrupts whose ISRs are allowed to preempt. */
  sldpDeviceNvicMaskDisable( sldpDevice );
#endif
  
  preempted = (SLDP_PreemptionContext_t*)ctx->pContextPreempted;
  preemptor = (SLDP_PreemptionContext_t*)ctx->pContextPreemptor;
  
  if ( (0==preempted) && (0==preemptor))
  {
    sldpDevice->pOwner = NULL;
    /* No contexts to become owner, disable device. */
    EFM_ASSERT(sldpDevice->disableDevice!=NULL);
    sldpDevice->disableDevice(sldpDevice->pDeviceHandle);
  }
  else
  {
    /* If _this_ context was preempted, and the preemptor is still running,
       then inform the preemptor that _this_ context is not valid any more
       by linking to _this_ preempted context (which may be NULL). */
    if (preemptor)
    {
      preemptor->pContextPreempted = preempted;
    }

    if (preempted)
    {
      /* Remove _this_ context from preemption list (double linked) by
         replacing preempted preemptor (_this_) with preemptor of _this_
         context (may be NULL if _this_ is owner of crypto). */
      preempted->pContextPreemptor = preemptor;

      /* If _this_ conxtext is the owner of crypto, restore preempted
         context and set it to owner. */
      if (sldpDevice->pOwner == ctx)
      {
        sldpDevice->pOwner = preempted;
        
        /* Load context values into DEVICE registers. */
        EFM_ASSERT(sldpDevice->loadContext!=NULL);
        sldpDevice->loadContext(sldpDevice->pDeviceHandle,
                                preempted->pDeviceContext);
        
#if defined( SLDP_DEVICE_YIELD_WHEN_NOT_OWNER )
        if (preempted->waitOwnership)
        {
          preempted->waitOwnership = false;
          int status = SLPAL_Complete(&preempted->ownership);
          EFM_ASSERT(status == 0);
        }
#endif
      }
    }
  }
  
#if defined( SLDP_ISR_PREEMPTION )
  /* Enable the interrupts whose ISRs are allowed to preempt. */
  sldpDeviceNvicMaskEnable( sldpDevice );
#endif
  
  if (false == RUNNING_AT_INTERRUPT_LEVEL)
  {
    SLPAL_GiveMutex( lock );
  }
  
  return 0;
}

/***************************************************************************//**
 * @brief
 *  Enter a critical section
 *
 * @details
 *  This function enters a critical section which protects the running context
 *  from being preempted by other threads by taking the device lock (mutex).
 *  Optionally the function will disable a set of user defined interrupts
 *  whose ISRs are allowed to preempt, in order to protect against these ISRs.
 *
 * @return
 *  0 for success, or negative value for error.
 *  
*/
int SLDP_CriticalEnter (SLDP_PreemptionContext_t *ctx)
{
  int ret = 0;
  
  if (false == RUNNING_AT_INTERRUPT_LEVEL)
  {
    ret = sldpWaitForOwnership( ctx );
  }
  
#if defined( SLDP_ISR_PREEMPTION )
  if (ret == 0)
  {
    SLDP_DeviceContext_t *sldpDevice = (SLDP_DeviceContext_t*) ctx->sldpDevice;
    /* Disable the interrupts whose ISRs are allowed to preempt. */
    sldpDeviceNvicMaskDisable( sldpDevice );
  }
#endif

  return( ret );
}

/***************************************************************************//**
 * @brief
 *  Exit a critical section.
 *
 * @details
 *  This function exits a critical section entered via the SLDP_EnterCritical
 *  function. This function will unlock the device (mutex), and optionally
 *  enable the set of user defined interrupts whose ISRs are allowed to preempt.
 *
 * @return
 *  0 for success.
 */
int SLDP_CriticalExit (SLDP_PreemptionContext_t *ctx)
{
  SLDP_DeviceContext_t *sldpDevice = (SLDP_DeviceContext_t*) ctx->sldpDevice;
  SLPAL_Mutex_t        *lock = &sldpDevice->lock;
  
#if defined( SLDP_ISR_PREEMPTION )
  /* Enable the interrupts whose ISRs are allowed to preempt. */
  sldpDeviceNvicMaskEnable( sldpDevice );
#endif
  
  if (false == RUNNING_AT_INTERRUPT_LEVEL)
  {
    SLPAL_GiveMutex( lock );
  }
  
  return( 0 );
}
