/**
 * \file slcl_device_aes.h
 *
 * \brief AES specific mbedtls slcl device definitions
 *
 *  Copyright (C) 2016, Silicon Labs, http://www.silabs.com
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

#ifndef MBEDTLS_SLCL_DEVICE_AES_H
#define MBEDTLS_SLCL_DEVICE_AES_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "em_device.h"

#if ( defined(AES_COUNT) && (AES_COUNT > 0) )

#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )
#include "sldp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/

#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )
typedef struct
{
  uint32_t CTRL;         /**< Control Register  */
  uint32_t IEN;          /**< Interrupt Enable Register  */
  uint32_t DATA[4];      /**< DATA Register  */
  uint32_t XORDATA;      /**< XORDATA Register  */
  uint32_t KEYLA;        /**< KEY Low Register  */
  uint32_t KEYLB;        /**< KEY Low Register  */
  uint32_t KEYLC;        /**< KEY Low Register  */
  uint32_t KEYLD;        /**< KEY Low Register  */
  uint32_t KEYHA;        /**< KEY High Register  */
  uint32_t KEYHB;        /**< KEY High Register  */
  uint32_t KEYHC;        /**< KEY High Register  */
  uint32_t KEYHD;        /**< KEY High Register  */  
} aes_context;

typedef struct
{
  SLDP_PreemptionContext_t sldp_ctx;    /**< Device preemption context */
  aes_context              aes_ctx;     /**< AES hw context */
} slcl_context;

#else

typedef void* slcl_context;

#endif

/*******************************************************************************
 *******************************   MACROS   ************************************
 ******************************************************************************/
#define AES_CLOCK_ENABLE    CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_AES
#define AES_CLOCK_DISABLE   CMU->HFCORECLKEN0 &= ~CMU_HFCORECLKEN0_AES
  
/*******************************************************************************
 ******************************* FUNCTIONS *************************************
 ******************************************************************************/

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
  (void) slcl_ctx;
  AES_CLOCK_ENABLE;
  return 0;
}

__STATIC_INLINE
int slcl_device_close( slcl_context * slcl_ctx )
{
  (void) slcl_ctx;
  AES_CLOCK_DISABLE;
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
          ( defined(AES_COUNT) && (AES_COUNT > 0) ) */

#endif /* MBEDTLS_SLCL_DEVICE_AES_H */
