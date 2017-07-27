// -----------------------------------------------------------------------------
// @file sniff.h
// @brief Sniff HAL configuration
//
// @section License
// <b>(C) Copyright 2016 Silicon Laboratories, http://www.silabs.com</b>
//
// This file is licensed under the Silabs License Agreement. See the file
// "Silabs_License_Agreement.txt" for details. Before using this software for
// any purpose, you must agree to the terms of that agreement.
//
// -----------------------------------------------------------------------------
#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "hal-config-types.h"
#include BOARD_HEADER

#define HAL_SERIAL_APP_PORT 0

#define HAL_SERIAL_VUART_ENABLE 1

#define HAL_WDOG_ENABLE 1

#endif //HAL_CONFIG_H
