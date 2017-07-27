/*
 *  AES specific mbedtls slcl device definitions
 *
 *  Copyright (C) 2017 Silicon Labs, http://www.silabs.com
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

#if defined(AES_COUNT) && (AES_COUNT > 0)

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "slcl_device_aes.h"
#include "slcl_device.h"
#include "em_assert.h"
#include <string.h>

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *  Initialize AES device specifics for given device instance
 *
 * @param devno
 *  AES device instance number.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 ******************************************************************************/
int mbedtls_device_specific_init (unsigned int devno)
{
  if (devno >= AES_COUNT)
    return( MBEDTLS_ERR_DEVICE_NOT_SUPPORTED );
  
  return( 0 );
}
  
/***************************************************************************//**
 * @brief
 *  Deinitialize AES device specifics for given device instance
 *
 * @param devno
 *  AES device instance number.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 ******************************************************************************/
int mbedtls_device_specific_free (unsigned int devno)
{
  if (devno >= AES_COUNT)
    return( MBEDTLS_ERR_DEVICE_NOT_SUPPORTED );
  else
    return( 0 );
}

#if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION )

/***************************************************************************//**
 * \brief
 *  Get device handle associated with a CRYPTO/AES device instance.
 *
 * \details
 *  This function returns the device handle associated with the given
 *  CRYPTO/AES device instance which can be used in subsequent calls to
 *  \ref mbedtls_device_enable, \ref mbedtls_device_enable,
 *  \ref mbedtls_device_context_store and \ref mbedtls_device_context_load.
 *
 * @param devno
 *  CRYPTO/AES device instance number.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 *****************************************************************************/
void* mbedtls_device_handle_get( unsigned int devno )
{
  if (devno >= AES_COUNT)
    return( (void*) 0 );
  else
    return( (void*) AES );
}

/***************************************************************************//**
 * @brief
 *  Enable given AES device instance
 *
 * @param devHandle
 *  Device handle.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device handle is invalid.
 ******************************************************************************/
int mbedtls_device_enable (void* devHandle)
{
  (void) devHandle;
  
  AES_CLOCK_ENABLE;
  
  return( 0 );
}
  
/***************************************************************************//**
 * @brief
 *  Disable given AES device instance
 *
 * @param devHandle
 *  Device handle.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device handle is invalid.
 ******************************************************************************/
int mbedtls_device_disable (void* devHandle)
{
  (void) devHandle;
  
  AES_CLOCK_DISABLE;
  
  return( 0 );
}

/***************************************************************************//**
 * @brief
 *  Store context of the AES device.
 *
 * @param dev
 *  Device pointer.
 *
 * @param dev_ctx
 *  Device context pointer.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 ******************************************************************************/
void mbedtls_device_context_store (void* dev, void* dev_ctx)
{
  aes_context *aes_ctx = (aes_context*) dev_ctx;
  
  EFM_ASSERT( dev == AES );

  /* Store values of the restorable AES registers */
  aes_ctx->CTRL    = AES->CTRL;
  aes_ctx->IEN     = AES->IEN;
  aes_ctx->DATA[0] = AES->DATA;
  aes_ctx->DATA[1] = AES->DATA;
  aes_ctx->DATA[2] = AES->DATA;
  aes_ctx->DATA[3] = AES->DATA;
  aes_ctx->KEYLA   = AES->KEYLA;
  aes_ctx->KEYLB   = AES->KEYLB;
  aes_ctx->KEYLC   = AES->KEYLC;
  aes_ctx->KEYLD   = AES->KEYLD;
  aes_ctx->KEYHA   = AES->KEYHA;
  aes_ctx->KEYHB   = AES->KEYHB;
  aes_ctx->KEYHC   = AES->KEYHC;
  aes_ctx->KEYHD   = AES->KEYHD;
}

/***************************************************************************//**
 * @brief
 *  Load context of the AES device.
 *
 * @param dev
 *  Device pointer.
 *
 * @param dev_ctx
 *  Device context pointer.
 *
 * @return
 *   0 if OK, or MBEDTLS_ERR_DEVICE_NOT_SUPPORTED if device number is invalid.
 ******************************************************************************/
void mbedtls_device_context_load (void* dev, void* dev_ctx)
{
  aes_context *aes_ctx = (aes_context*) dev_ctx;
  
  EFM_ASSERT( dev == AES );

  /* Load AES registers with values stored in context structure.
     We need to write data first in case the DATASTART or XORSTART bits in
     AES->CTRL is set to start encryption/decryption when DATA3 is written. */
  AES->CTRL    =  0;
  AES->DATA    =  aes_ctx->DATA[0];
  AES->DATA    =  aes_ctx->DATA[1];
  AES->DATA    =  aes_ctx->DATA[2];
  AES->DATA    =  aes_ctx->DATA[3];
  
  AES->CTRL    =  aes_ctx->CTRL;
  AES->IEN     =  aes_ctx->IEN;
  AES->KEYLA   =  aes_ctx->KEYLA;
  AES->KEYLB   =  aes_ctx->KEYLB;
  AES->KEYLC   =  aes_ctx->KEYLC;
  AES->KEYLD   =  aes_ctx->KEYLD;
  AES->KEYHA   =  aes_ctx->KEYHA;
  AES->KEYHB   =  aes_ctx->KEYHB;
  AES->KEYHC   =  aes_ctx->KEYHC;
  AES->KEYHD   =  aes_ctx->KEYHD;
}

#endif /* #if defined( MBEDTLS_CRYPTO_DEVICE_PREEMPTION ) */

#endif /* #if defined(AES_COUNT) && (AES_COUNT > 0) */
