/**
 * \file slcl_device_crypto.h
 *
 * \brief CRYPTO specific mbedtls slcl device definitions
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

#ifndef MBEDTLS_SLCL_DEVICE_CRYPTO_H
#define MBEDTLS_SLCL_DEVICE_CRYPTO_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "em_device.h"

#if ( defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) )

#include "slcl_device.h"

#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )
#include "sldp.h"
#endif
#include "em_crypto.h"
#include "dmadrv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *******************************   MACROS   ************************************
 ******************************************************************************/

#define SLCL_DEVICE_CRITICAL_ENTER(ctx) do { status = slcl_device_critical_enter( ctx ); EFM_ASSERT( status == 0 ); } while( 0 )
#define SLCL_DEVICE_CRITICAL_EXIT(ctx) do { status = slcl_device_critical_exit( ctx ); EFM_ASSERT( status == 0 ); } while( 0 )

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/

#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )
  
/** Preemptable context of CRYPTO hardware module. */
typedef struct
{
  uint32_t CTRL;          /*!< Control Register  */
  uint32_t WAC;           /*!< Wide Arithmetic Configuration  */
  uint32_t SEQCTRL;       /*!< Sequence Control  */
  uint32_t SEQCTRLB;      /*!< Sequence Control B  */
  uint32_t IEN;           /*!< Interrupt Enable Register  */
  uint32_t SEQ[5];        /*!< Instruction Sequence registers */
  CRYPTO_Data260_TypeDef DDATA[5]; /*!< DDATA registers. Covers all data
                                      registers
                                      of CRYPTO, including DATA(128 bit),
                                      DDATA (256bit/260bit),
                                      QDATA (512bit) registers. */
} crypto_context;

#endif

/** CRYPTO device structure. */
typedef struct
{
  CRYPTO_TypeDef           *crypto;          /*!< CRYPTO hw instance */
  IRQn_Type                 irqn;            /*!< CRYPTO irq number */
  uint32_t                  clk;             /*!< CRYPTO clock */
  mbedtls_device_context  **pp_mbedtls_device_ctx; /*!< Pointer to pointer of
                                                     mbedtls device context */
#if defined(MBEDTLS_INCLUDE_IO_MODE_DMA)
  DMADRV_PeripheralSignal_t dmaReqSigChIn;   /*!< DMA req signal channel in */
  DMADRV_PeripheralSignal_t dmaReqSigChOut;  /*!< DMA req signal channel out */
#endif
} crypto_device_context;

/** SLCL operation context structure */
typedef struct
{
    const crypto_device_context *crypto_device_ctx; /*!< Pointer ref to crypto
                                                      device context. */
#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )
    SLDP_PreemptionContext_t sldp_ctx;        /*!< Device preemption context */
    crypto_context           crypto_ctx;      /*!< Preemptable CRYPTO hw
                                                context */
#endif
} slcl_context;

/*******************************************************************************
 ******************************* FUNCTIONS *************************************
 ******************************************************************************/

/**
 * \brief
 *   Select which CRYPTO device instance to use in CRYPTO context.
 *
 * \param slcl_ctx
 *  Pointer to SLCL context.
 *
 * \param devno
 *  CRYPTO device instance number.
 *
 * \return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 */
int crypto_device_instance_set(slcl_context * slcl_ctx,
                               unsigned int   devno);

/**
 * \brief
 *   Get pointer to crypto device context associated with given SLCL context.
 *
 * \param slcl_ctx
 *  Pointer to SLCL context.
 *
 * \return
 *   CRYPTO device context pointer
 */
__STATIC_INLINE
const crypto_device_context* crypto_device_context_get( slcl_context *slcl_ctx )
{
  return slcl_ctx->crypto_device_ctx;
}

/**
 * \brief
 *   Get pointer to the crypto hardware unit associated with given SLCL context.
 *
 * \param slcl_ctx
 *  Pointer to SLCL context.
 *
 * \return
 *   CRYPTO hardware pointer
 */
__STATIC_INLINE
CRYPTO_TypeDef* crypto_get( slcl_context * slcl_ctx )
{
  return slcl_ctx->crypto_device_ctx->crypto;
}

#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )

__STATIC_INLINE
int slcl_device_open( slcl_context * slcl_ctx )
{
  return SLDP_Arbitrate( &slcl_ctx->sldp_ctx );
}

__STATIC_INLINE
int slcl_device_close( slcl_context * slcl_ctx )
{
  return SLDP_Release( &slcl_ctx->sldp_ctx );
}

__STATIC_INLINE
int slcl_device_critical_enter( slcl_context * slcl_ctx )
{
  return SLDP_CriticalEnter( &slcl_ctx->sldp_ctx );
}

__STATIC_INLINE
int slcl_device_critical_exit( slcl_context * slcl_ctx )
{
  return SLDP_CriticalExit( &slcl_ctx->sldp_ctx );
}

#else  /* #if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION ) */

__STATIC_INLINE
int slcl_device_open( slcl_context * slcl_ctx )
{
  /* Start the CRYPTO clock */
  CMU->HFBUSCLKEN0 |= slcl_ctx->crypto_device_ctx->clk;
  return 0;
}

__STATIC_INLINE
int slcl_device_close( slcl_context * slcl_ctx )
{
  CMU->HFBUSCLKEN0 &= ~slcl_ctx->crypto_device_ctx->clk;
  return 0;
}

__STATIC_INLINE
int slcl_device_critical_enter( slcl_context * slcl_ctx )
{
  (void) slcl_ctx;
  return 0;
}

__STATIC_INLINE
int slcl_device_critical_exit( slcl_context * slcl_ctx )
{
  (void) slcl_ctx;
  return 0;
}

#endif /* #if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION ) */

#ifdef __cplusplus
}
#endif

#endif /* #if ( defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) ) || \
          ( defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) ) */

#endif /* MBEDTLS_SLCL_DEVICE_CRYPTO_H */
