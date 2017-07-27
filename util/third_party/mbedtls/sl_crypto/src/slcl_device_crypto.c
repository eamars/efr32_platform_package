/*
 *  CRYPTO specific mbedtls slcl device implementation
 *
 *  Copyright (C) 2017, Silicon Labs, httgp://www.silabs.com
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
#include "em_device.h"

#if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0)

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "slcl_device_crypto.h"
#include "slcl_device.h"
#include "sl_crypto_internal.h"

/*******************************************************************************
 ********************************   STATICS   **********************************
 ******************************************************************************/

#if !defined( MBEDTLS_DEVICE_INIT_INTERNAL_DISABLE )
/* Device structures for internal initialization in order for backwards
   compatibility. Use MBEDTLS_DEVICE_INIT_INTERNAL_DISABLE to disable
   the internal device initialization. Internal initialization consumes
   more RAM because the device structures are instantiated statically here,
   inside the mbedTLS library. */
static mbedtls_device_context _mbedtls_device_ctx[MBEDTLS_DEVICE_COUNT];
#endif

/* CRYPTO device instance structures. */
static const crypto_device_context crypto_device_ctx[CRYPTO_COUNT] =
{
#if defined( CRYPTO0 )
  {
    CRYPTO0,
    CRYPTO0_IRQn,
    CMU_HFBUSCLKEN0_CRYPTO0,
    &p_mbedtls_device_context[0]
#if defined( MBEDTLS_INCLUDE_IO_MODE_DMA )
    ,
    dmadrvPeripheralSignal_CRYPTO0_DATA0WR,
    dmadrvPeripheralSignal_CRYPTO0_DATA0RD
#endif
  }
#elif defined( CRYPTO )
  {
    CRYPTO,
    CRYPTO_IRQn,
    CMU_HFBUSCLKEN0_CRYPTO,
    &p_mbedtls_device_context[0]
#if defined( MBEDTLS_INCLUDE_IO_MODE_DMA )
    ,
    dmadrvPeripheralSignal_CRYPTO_DATA0WR,
    dmadrvPeripheralSignal_CRYPTO_DATA0RD
#endif
  }
#endif
#if defined( CRYPTO1 )
  ,
  {
    CRYPTO1,
    CRYPTO1_IRQn,
    CMU_HFBUSCLKEN0_CRYPTO1,
    &p_mbedtls_device_context[1]
#if defined( MBEDTLS_INCLUDE_IO_MODE_DMA )
    ,
    dmadrvPeripheralSignal_CRYPTO1_DATA0WR,
    dmadrvPeripheralSignal_CRYPTO1_DATA0RD
#endif
  }
#endif
};

/*******************************************************************************
 ******************************   FUNCTIONS   **********************************
 ******************************************************************************/

/**
 * \brief
 *   Initialize CRYPTO device specifics for given device instance
 *
 * \param devno
 *  CRYPTO device instance number.
 *
 * \return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 */
int mbedtls_device_specific_init (unsigned int devno)
{
#if defined( MBEDTLS_DEVICE_YIELD_WHEN_BUSY )

  const crypto_device_context* ctx;
  CRYPTO_TypeDef*              crypto;
  IRQn_Type                    irqn;
  uint32_t                     clk;
  
  if (devno >= CRYPTO_COUNT)
    return( MBEDTLS_ERR_DEVICE_NOT_SUPPORTED );
  
  ctx    = &crypto_device_ctx[devno];
  crypto = ctx->crypto;
  irqn   = ctx->irqn;
  clk    = ctx->clk;
  
  /* Enable CRYPTO clock in order to clear interrupts. */
  CRYPTO_CLOCK_ENABLE(clk);
  
  crypto->IFC = _CRYPTO_IFC_MASK;
  
  NVIC_ClearPendingIRQ(irqn);
  NVIC_EnableIRQ(irqn);
  NVIC_SetPriority(irqn, SLPAL_CRYPTO_IRQ_PRIORITY);

  CRYPTO_CLOCK_DISABLE(clk);

#else
  
  (void) devno;
  
#endif
  
  return( 0 );
}

/**
 * \brief
 *   Deinitialize CRYPTO device specifics for given device instance
 *
 * \param devno
 *  CRYPTO device instance number.
 *
 * \return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 */
int mbedtls_device_specific_free (unsigned int devno)
{
#if defined( MBEDTLS_DEVICE_YIELD_WHEN_BUSY )

  const crypto_device_context *ctx;
  CRYPTO_TypeDef              *crypto;
  IRQn_Type                    irqn;
  uint32_t                     clk;
  
  if (devno >= CRYPTO_COUNT)
    return( MBEDTLS_ERR_DEVICE_NOT_SUPPORTED );
  
  ctx    = &crypto_device_ctx[devno];
  crypto = ctx->crypto;
  irqn   = ctx->irqn;
  clk    = ctx->clk;
  
  /* Enable CRYPTO clock in order to clear interrupts. */
  CRYPTO_CLOCK_ENABLE(clk);
  
  crypto->IEN = 0;
  crypto->IFC = _CRYPTO_IFC_MASK;
  
  NVIC_DisableIRQ(irqn);
    
  CRYPTO_CLOCK_DISABLE(clk);
  
#else
  
  (void) devno;
  
#endif
  
  return( 0 );
}

#ifdef MBEDTLS_CRYPTO_DEVICE_PREEMPTION

/*
 * \brief
 *  Get device handle associated with a CRYPTO/AES device instance.
 *
 * \details
 *  This function returns the device handle associated with the given
 *  CRYPTO/AES device instance which can be used in subsequent calls to
 *  \ref mbedtls_device_enable, \ref mbedtls_device_enable,
 *  \ref mbedtls_device_context_store and \ref mbedtls_device_context_load.
 *
 * \param devno
 *  CRYPTO/AES device instance number.
 *
 * \return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 **/
void* mbedtls_device_handle_get( unsigned int devno )
{
    if ( devno > CRYPTO_COUNT )
        return( (void*) 0 );
    else
        return (void*) &crypto_device_ctx[devno];
}

/*
 * \brief
 *  Enable given CRYPTO device instance
 *
 * \param dev_handle
 *  Device handle.
 *
 * \return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device handle is invalid.
 **/
int mbedtls_device_enable( void* dev_handle )
{
    crypto_device_context *ctx = (crypto_device_context *) dev_handle;
  
    /* Enable CRYPTO clock */
    CRYPTO_CLOCK_ENABLE(ctx->clk);
  
    return( 0 );
}
  
/*
 * \brief
 *  Disable given AES device instance
 *
 * \param dev_handle
 *  Device handle.
 *
 * \return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device handle is invalid.
 **/
int mbedtls_device_disable (void* dev_handle)
{
    crypto_device_context *ctx = (crypto_device_context *) dev_handle;
  
    /* Disable CRYPTO clock */
    CRYPTO_CLOCK_DISABLE(ctx->clk);
  
    return( 0 );
}

/*
 * \brief
 *  Store context of the CRYPTO device.
 *
 * \param dev_handle
 *  Device handle.
 *
 * \param dev_ctx
 *  Device context pointer.
 *
 * \return
 *   N/A
 **/
void mbedtls_device_context_store (void* dev_handle, void* dev_ctx)
{
    crypto_device_context *crypto_device_ctx =
      (crypto_device_context *) dev_handle;
    CRYPTO_TypeDef *crypto     = crypto_device_ctx->crypto;
    crypto_context *crypto_ctx = (crypto_context*)  dev_ctx;
    uint32_t        wac        = crypto->WAC;
    uint8_t*        exec_cmd;
    
    crypto_ctx->WAC      = wac;
    crypto_ctx->CTRL     = crypto->CTRL;
    crypto_ctx->SEQCTRL  = crypto->SEQCTRL;
    crypto_ctx->SEQCTRLB = crypto->SEQCTRLB;
    crypto_ctx->IEN      = crypto->IEN;
    crypto_ctx->SEQ[0]   = crypto->SEQ0;
    crypto_ctx->SEQ[1]   = crypto->SEQ1;
    crypto_ctx->SEQ[2]   = crypto->SEQ2;
    crypto_ctx->SEQ[3]   = crypto->SEQ3;
    crypto_ctx->SEQ[4]   = crypto->SEQ4;
    
    if ( (wac & _CRYPTO_WAC_RESULTWIDTH_MASK) == CRYPTO_WAC_RESULTWIDTH_260BIT)
    {
        CRYPTO_DData0Read260(crypto, crypto_ctx->DDATA[0]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA1TODDATA0; /* Move DDATA1 to DDATA0
                                                          in order to read. */
        CRYPTO_DData0Read260(crypto, crypto_ctx->DDATA[1]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA2TODDATA0; /* Move DDATA2 to DDATA0
                                                          in order to read. */
        CRYPTO_DData0Read260(crypto, crypto_ctx->DDATA[2]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA3TODDATA0; /* Move DDATA3 to DDATA0
                                                          in order to read. */
        CRYPTO_DData0Read260(crypto, crypto_ctx->DDATA[3]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA4TODDATA0; /* Move DDATA4 to DDATA0
                                                          in order to read. */
        CRYPTO_DData0Read260(crypto, crypto_ctx->DDATA[4]);
    }
    else
    {
        CRYPTO_DDataRead(&crypto->DDATA0, crypto_ctx->DDATA[0]);
        CRYPTO_DDataRead(&crypto->DDATA1, crypto_ctx->DDATA[1]);
        CRYPTO_DDataRead(&crypto->DDATA2, crypto_ctx->DDATA[2]);
        CRYPTO_DDataRead(&crypto->DDATA3, crypto_ctx->DDATA[3]);
        CRYPTO_DDataRead(&crypto->DDATA4, crypto_ctx->DDATA[4]);
    }

    
    /* Search for possible EXEC commands and replace with END. */
    exec_cmd = (uint8_t*) memchr(&crypto_ctx->SEQ,
                                 CRYPTO_CMD_INSTR_EXEC,
                                 sizeof(crypto_ctx->SEQ));
    if (exec_cmd)
    {
        memset(exec_cmd,
               (uint8_t) CRYPTO_CMD_INSTR_END,
               sizeof(crypto_ctx->SEQ) -
               ((uint32_t)exec_cmd-(uint32_t)&crypto_ctx->SEQ));
    }
}

/*
 * \brief
 *  Load context of the CRYPTO device.
 *
 * \param dev_handle
 *  Device handle.
 *
 * \param dev_ctx
 *  Device context pointer.
 *
 * \return
 *   N/A
 **/
void mbedtls_device_context_load (void* dev_handle, void* dev_ctx)
{
    crypto_device_context *crypto_device_ctx =
      (crypto_device_context *) dev_handle;
    CRYPTO_TypeDef *crypto     = crypto_device_ctx->crypto;
    crypto_context *crypto_ctx = (crypto_context*)  dev_ctx;
    uint32_t wac     = crypto_ctx->WAC;
    
    crypto->WAC      = wac;
    crypto->CTRL     = crypto_ctx->CTRL;
    crypto->SEQCTRL  = crypto_ctx->SEQCTRL;
    crypto->SEQCTRLB = crypto_ctx->SEQCTRLB;
    crypto->IEN      = crypto_ctx->IEN;
    crypto->SEQ0     = crypto_ctx->SEQ[0];
    crypto->SEQ1     = crypto_ctx->SEQ[1];
    crypto->SEQ2     = crypto_ctx->SEQ[2];
    crypto->SEQ3     = crypto_ctx->SEQ[3];
    crypto->SEQ4     = crypto_ctx->SEQ[4];
     
    if ( (wac & _CRYPTO_WAC_RESULTWIDTH_MASK) == CRYPTO_WAC_RESULTWIDTH_260BIT)
    {
        /* Start by writing the DDATA1 value to DDATA0 and move to DDATA1. */
        CRYPTO_DData0Write260(crypto, crypto_ctx->DDATA[1]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA0TODDATA1;
        
        /* Write the DDATA2 value to DDATA0 and move to DDATA2. */
        CRYPTO_DData0Write260(crypto, crypto_ctx->DDATA[2]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA0TODDATA2;
        
        /* Write the DDATA3 value to DDATA0 and move to DDATA3. */
        CRYPTO_DData0Write260(crypto, crypto_ctx->DDATA[3]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA0TODDATA3;
        
        /* Write the DDATA4 value to DDATA0 and move to DDATA4. */
        CRYPTO_DData0Write260(crypto, crypto_ctx->DDATA[4]);
        crypto->CMD = CRYPTO_CMD_INSTR_DDATA0TODDATA4;
        
        /* Finally write DDATA0 */
        CRYPTO_DData0Write260(crypto, crypto_ctx->DDATA[0]);
    }
    else
    {
        CRYPTO_DDataWrite(&crypto->DDATA0, crypto_ctx->DDATA[0]);
        CRYPTO_DDataWrite(&crypto->DDATA1, crypto_ctx->DDATA[1]);
        CRYPTO_DDataWrite(&crypto->DDATA2, crypto_ctx->DDATA[2]);
        CRYPTO_DDataWrite(&crypto->DDATA3, crypto_ctx->DDATA[3]);
        CRYPTO_DDataWrite(&crypto->DDATA4, crypto_ctx->DDATA[4]);
    }
}

#endif

/*
 * \brief
 *  Set device instance of SLCL device.
 *
 * \param dev_ctx
 *  Device context pointer.
 *
 * \param devno
 *  Device instance number.
 *
 * \return
 *   0 if ok, or error code, ref slcl_device.h.
 **/
int crypto_device_instance_set(slcl_context * slcl_ctx,
                               unsigned int   devno)
{
    if (devno >= CRYPTO_COUNT)
        return( MBEDTLS_ERR_DEVICE_NOT_SUPPORTED );

#if !defined( MBEDTLS_DEVICE_INIT_INTERNAL_DISABLE )
    /* Initialize the mbedtls device structures internally for backwards
       compatibility. Use MBEDTLS_DEVICE_INIT_INTERNAL_DISABLE to disable
       the internal device initialization. Internal initialization consumes
       more RAM because the device structures are instantiated statically
       inside the mbedTLS library. */
    if (p_mbedtls_device_context[devno] == 0)
    {
        mbedtls_device_init(&_mbedtls_device_ctx[devno]);
        mbedtls_device_set_instance(&_mbedtls_device_ctx[devno], devno);
    }
#endif

    /* Check that the specified device is initialized. */
    if (p_mbedtls_device_context[devno] == 0)
        return( MBEDTLS_ERR_DEVICE_NOT_INITIALIZED );
    
#if defined(MBEDTLS_CRYPTO_DEVICE_PREEMPTION)
    /* Associate SLDP context with specified SLDP device. */
    SLDP_ContextDeviceSet( &slcl_ctx->sldp_ctx,
                           &p_mbedtls_device_context[devno]->sldp_dev_ctx );
#endif

    /* Associate context with specified CRYPTO device. */
    slcl_ctx->crypto_device_ctx = &crypto_device_ctx[devno];

    return( 0 );
}

#if defined(MBEDTLS_DEVICE_YIELD_WHEN_BUSY)
                              
/**
 * \brief
 *  Interrupt service routine for CRYPTO module instances.
 *
 * \details
 *  CRYPTO_IRQHandler, CRYPTO0_IRQHandler, etc are the first functions to be
 *  called when an interrupt from the respective CRYPTO instance is being
 *  serviced by the MCU. The function crypto_device_irq_handler is called with
 *  a pointer to the respective CRYPTO device structure, and it will clear the
 *  interrupt and signal the event.
 *
 * \return
 *  N/A
 */
void crypto_device_irq_handler( const crypto_device_context *ctx )
{
    CRYPTO_TypeDef* crypto = ctx->crypto;
    uint32_t flags = crypto->IF;
    mbedtls_device_context *p_mbedtls_dev_ctx;
    int status;

    while (flags)
    {
        crypto->IFC = flags;
        NVIC_ClearPendingIRQ(ctx->irqn);
        
        if (flags & CRYPTO_IF_SEQDONE)
        {
            p_mbedtls_dev_ctx = *ctx->pp_mbedtls_device_ctx;
            EFM_ASSERT( p_mbedtls_dev_ctx != 0 );
            status = SLPAL_Complete(&p_mbedtls_dev_ctx->operation);
            EFM_ASSERT(status == 0);
        }
        if (CMU->HFBUSCLKEN0 & ctx->clk)
          flags = crypto->IF;
        else
          flags = 0;
    }
}

#if defined(CRYPTO)
void CRYPTO_IRQHandler(void)
{
    SLPAL_IsrEnter();
    crypto_device_irq_handler( &crypto_device_ctx[0] );
    SLPAL_IsrExit();
}
#endif

#if defined(CRYPTO0)
void CRYPTO0_IRQHandler(void)
{
    SLPAL_IsrEnter();
    crypto_device_irq_handler( &crypto_device_ctx[0] );
    SLPAL_IsrExit();
}
#endif

#if defined(CRYPTO1)
void CRYPTO1_IRQHandler(void)
{
    SLPAL_IsrEnter();
    crypto_device_irq_handler( &crypto_device_ctx[1] );
    SLPAL_IsrExit();
}
#endif

#endif /* #if defined(MBEDTLS_DEVICE_YIELD_WHEN_BUSY) */

#endif /* #if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) */
