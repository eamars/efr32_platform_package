/***************************************************************************//**
 * @file dmadrv_config.h
 * @brief DMADRV configuration file.
 * @version 4.2.3
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2015 Silicon Labs, www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/
#ifndef __SILICON_LABS_DMADRV_CONFIG_H__
#define __SILICON_LABS_DMADRV_CONFIG_H__

/***************************************************************************//**
 * @addtogroup EM_Drivers
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup DMADRV
 * @{
 ******************************************************************************/

/// DMADRV DMA interrupt priority configuration option.
/// Set DMA interrupt priority. Range is 0..7, 0 is highest priority.
#ifndef EMDRV_DMADRV_DMA_IRQ_PRIORITY
#define EMDRV_DMADRV_DMA_IRQ_PRIORITY 4
#endif

/// DMADRV DMA channel priority configuration option.
/// Set DMA channel priority. Range 0..EMDRV_DMADRV_DMA_CH_COUNT.
/// On LDMA, this will configure channel 0 to CH_PRIORITY - 1 as fixed priority,
/// and CH_PRIORITY to CH_COUNT as round-robin.
/// On DMA, this will have no impact, since high priority is unuseable with
/// peripherals.
#ifndef EMDRV_DMADRV_DMA_CH_PRIORITY
#define EMDRV_DMADRV_DMA_CH_PRIORITY 0
#endif

/// DMADRV channel count configuration option.
/// Number of DMA channels to support. A lower DMA channel count will reduce
/// ram memory footprint.
#define EMDRV_DMADRV_DMA_CH_COUNT 8

/// DMADRV native API configuration option.
/// Use the native emlib api of the DMA controller, but still use DMADRV
/// housekeeping functions as AllocateChannel/FreeChannel etc.
#if defined(DOXY_DOC_ONLY)
#define EMDRV_DMADRV_USE_NATIVE_API
#else
//#define EMDRV_DMADRV_USE_NATIVE_API
#endif

/** @} (end addtogroup DMADRV) */
/** @} (end addtogroup EM_Drivers) */

#endif /* __SILICON_LABS_DMADRV_CONFIG_H__ */
