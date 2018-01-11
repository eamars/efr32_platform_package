/**************************************************************************//**
 * @file efr32fg1p_af_ports.h
 * @brief EFR32FG1P_AF_PORTS register and bit field definitions
 * @version 5.3.5
 ******************************************************************************
 * # License
 * <b>Copyright 2017 Silicon Laboratories, Inc. www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/

#if defined(__ICCARM__)
#pragma system_include       /* Treat file as system include file. */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang system_header  /* Treat file as system include file. */
#endif

/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @addtogroup EFR32FG1P_Alternate_Function Alternate Function
 * @{
 * @defgroup EFR32FG1P_AF_Ports Alternate Function Ports
 * @{
 *****************************************************************************/

#define AF_CMU_CLK0_PORT(i)         ((i) == 0 ? 0 : (i) == 1 ? 1 : (i) == 2 ? 2 : (i) == 3 ? 2 : (i) == 4 ? 3 : (i) == 5 ? 3 : (i) == 6 ? 5 : (i) == 7 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                               /**< Port number for AF_CMU_CLK0 location number i */
#define AF_CMU_CLK1_PORT(i)         ((i) == 0 ? 0 : (i) == 1 ? 1 : (i) == 2 ? 2 : (i) == 3 ? 2 : (i) == 4 ? 3 : (i) == 5 ? 3 : (i) == 6 ? 5 : (i) == 7 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                               /**< Port number for AF_CMU_CLK1 location number i */
#define AF_PRS_CH0_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 : (i) == 3 ? 5 : (i) == 4 ? 5 : (i) == 5 ? 5 : (i) == 6 ? 5 : (i) == 7 ? 5 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 :  -1)                                                                                                                                                                                                                                                                                                 /**< Port number for AF_PRS_CH0 location number i */
#define AF_PRS_CH1_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 : (i) == 3 ? 5 : (i) == 4 ? 5 : (i) == 5 ? 5 : (i) == 6 ? 5 : (i) == 7 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                               /**< Port number for AF_PRS_CH1 location number i */
#define AF_PRS_CH2_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 : (i) == 3 ? 5 : (i) == 4 ? 5 : (i) == 5 ? 5 : (i) == 6 ? 5 : (i) == 7 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                               /**< Port number for AF_PRS_CH2 location number i */
#define AF_PRS_CH3_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 5 : (i) == 2 ? 5 : (i) == 3 ? 5 : (i) == 4 ? 5 : (i) == 5 ? 5 : (i) == 6 ? 5 : (i) == 7 ? 5 : (i) == 8 ? 3 : (i) == 9 ? 3 : (i) == 10 ? 3 : (i) == 11 ? 3 : (i) == 12 ? 3 : (i) == 13 ? 3 : (i) == 14 ? 3 :  -1)                                                                                                                                                                                                                                                                                 /**< Port number for AF_PRS_CH3 location number i */
#define AF_PRS_CH4_PORT(i)          ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 : (i) == 3 ? 3 : (i) == 4 ? 3 : (i) == 5 ? 3 : (i) == 6 ? 3 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                              /**< Port number for AF_PRS_CH4 location number i */
#define AF_PRS_CH5_PORT(i)          ((i) == 0 ? 3 : (i) == 1 ? 3 : (i) == 2 ? 3 : (i) == 3 ? 3 : (i) == 4 ? 3 : (i) == 5 ? 3 : (i) == 6 ? 3 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                              /**< Port number for AF_PRS_CH5 location number i */
#define AF_PRS_CH6_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 3 : (i) == 12 ? 3 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 :  -1)                                                                                                                                                                                                                                 /**< Port number for AF_PRS_CH6 location number i */
#define AF_PRS_CH7_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 0 :  -1)                                                                                                                                                                                                                                                                                                                                                 /**< Port number for AF_PRS_CH7 location number i */
#define AF_PRS_CH8_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 0 : (i) == 10 ? 0 :  -1)                                                                                                                                                                                                                                                                                                                                                 /**< Port number for AF_PRS_CH8 location number i */
#define AF_PRS_CH9_PORT(i)          ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 0 : (i) == 9 ? 0 : (i) == 10 ? 0 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 :  -1)                                                                                                                                                                                                                                                 /**< Port number for AF_PRS_CH9 location number i */
#define AF_PRS_CH10_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 : (i) == 3 ? 2 : (i) == 4 ? 2 : (i) == 5 ? 2 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                             /**< Port number for AF_PRS_CH10 location number i */
#define AF_PRS_CH11_PORT(i)         ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? 2 : (i) == 3 ? 2 : (i) == 4 ? 2 : (i) == 5 ? 2 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                             /**< Port number for AF_PRS_CH11 location number i */
#define AF_TIMER0_CC0_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_TIMER0_CC0 location number i */
#define AF_TIMER0_CC1_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER0_CC1 location number i */
#define AF_TIMER0_CC2_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER0_CC2 location number i */
#define AF_TIMER0_CC3_PORT(i)       (-1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_TIMER0_CC3 location number i */
#define AF_TIMER0_CDTI0_PORT(i)     ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER0_CDTI0 location number i */
#define AF_TIMER0_CDTI1_PORT(i)     ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 2 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 5 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 0 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER0_CDTI1 location number i */
#define AF_TIMER0_CDTI2_PORT(i)     ((i) == 0 ? 0 : (i) == 1 ? 1 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 2 : (i) == 7 ? 2 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 3 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 5 : (i) == 20 ? 5 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 0 : (i) == 28 ? 0 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER0_CDTI2 location number i */
#define AF_TIMER0_CDTI3_PORT(i)     (-1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_TIMER0_CDTI3 location number i */
#define AF_TIMER1_CC0_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_TIMER1_CC0 location number i */
#define AF_TIMER1_CC1_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER1_CC1 location number i */
#define AF_TIMER1_CC2_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER1_CC2 location number i */
#define AF_TIMER1_CC3_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_TIMER1_CC3 location number i */
#define AF_TIMER1_CDTI0_PORT(i)     (-1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_TIMER1_CDTI0 location number i */
#define AF_TIMER1_CDTI1_PORT(i)     (-1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_TIMER1_CDTI1 location number i */
#define AF_TIMER1_CDTI2_PORT(i)     (-1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_TIMER1_CDTI2 location number i */
#define AF_TIMER1_CDTI3_PORT(i)     (-1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_TIMER1_CDTI3 location number i */
#define AF_USART0_TX_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_USART0_TX location number i */
#define AF_USART0_RX_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART0_RX location number i */
#define AF_USART0_CLK_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART0_CLK location number i */
#define AF_USART0_CS_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART0_CS location number i */
#define AF_USART0_CTS_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 2 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 5 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 0 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART0_CTS location number i */
#define AF_USART0_RTS_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 1 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 2 : (i) == 7 ? 2 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 3 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 5 : (i) == 20 ? 5 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 0 : (i) == 28 ? 0 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART0_RTS location number i */
#define AF_USART1_TX_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_USART1_TX location number i */
#define AF_USART1_RX_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART1_RX location number i */
#define AF_USART1_CLK_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART1_CLK location number i */
#define AF_USART1_CS_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART1_CS location number i */
#define AF_USART1_CTS_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 2 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 5 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 0 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART1_CTS location number i */
#define AF_USART1_RTS_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 1 : (i) == 2 ? 1 : (i) == 3 ? 1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 2 : (i) == 7 ? 2 : (i) == 8 ? 2 : (i) == 9 ? 2 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 3 : (i) == 13 ? 3 : (i) == 14 ? 3 : (i) == 15 ? 3 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 5 : (i) == 20 ? 5 : (i) == 21 ? 5 : (i) == 22 ? 5 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 0 : (i) == 28 ? 0 : (i) == 29 ? 0 : (i) == 30 ? 0 : (i) == 31 ? 0 :  -1) /**< Port number for AF_USART1_RTS location number i */
#define AF_LEUART0_TX_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_LEUART0_TX location number i */
#define AF_LEUART0_RX_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_LEUART0_RX location number i */
#define AF_LETIMER0_OUT0_PORT(i)    ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_LETIMER0_OUT0 location number i */
#define AF_LETIMER0_OUT1_PORT(i)    ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_LETIMER0_OUT1 location number i */
#define AF_PCNT0_S0IN_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_PCNT0_S0IN location number i */
#define AF_PCNT0_S1IN_PORT(i)       ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_PCNT0_S1IN location number i */
#define AF_I2C0_SDA_PORT(i)         ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_I2C0_SDA location number i */
#define AF_I2C0_SCL_PORT(i)         ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 2 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 3 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 5 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 0 :  -1) /**< Port number for AF_I2C0_SCL location number i */
#define AF_ACMP0_OUT_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_ACMP0_OUT location number i */
#define AF_ACMP1_OUT_PORT(i)        ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? 0 : (i) == 3 ? 0 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 : (i) == 7 ? 1 : (i) == 8 ? 1 : (i) == 9 ? 1 : (i) == 10 ? 1 : (i) == 11 ? 2 : (i) == 12 ? 2 : (i) == 13 ? 2 : (i) == 14 ? 2 : (i) == 15 ? 2 : (i) == 16 ? 2 : (i) == 17 ? 3 : (i) == 18 ? 3 : (i) == 19 ? 3 : (i) == 20 ? 3 : (i) == 21 ? 3 : (i) == 22 ? 3 : (i) == 23 ? 3 : (i) == 24 ? 5 : (i) == 25 ? 5 : (i) == 26 ? 5 : (i) == 27 ? 5 : (i) == 28 ? 5 : (i) == 29 ? 5 : (i) == 30 ? 5 : (i) == 31 ? 5 :  -1) /**< Port number for AF_ACMP1_OUT location number i */
#define AF_DBG_TDI_PORT(i)          ((i) == 0 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_DBG_TDI location number i */
#define AF_DBG_TDO_PORT(i)          ((i) == 0 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_DBG_TDO location number i */
#define AF_DBG_SWV_PORT(i)          ((i) == 0 ? 5 : (i) == 1 ? 1 : (i) == 2 ? 3 : (i) == 3 ? 2 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                                                           /**< Port number for AF_DBG_SWV location number i */
#define AF_DBG_SWDIOTMS_PORT(i)     ((i) == 0 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_DBG_SWDIOTMS location number i */
#define AF_DBG_SWCLKTCK_PORT(i)     ((i) == 0 ? 5 :  -1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /**< Port number for AF_DBG_SWCLKTCK location number i */

/** @} */
/** @} End of group EFR32FG1P_AF_Ports */
/** @} End of group Parts */
