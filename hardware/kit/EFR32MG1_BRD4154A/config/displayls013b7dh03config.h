/**************************************************************************//**
* @file displayls013b7dh03config.h
* @brief BRD4154A specific configuration for the display driver for
*        the Sharp Memory LCD model LS013B7DH03.
* @version 5.3.3
******************************************************************************
* # License
* <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
*******************************************************************************
*
* This file is licensed under the Silicon Labs License Agreement. See the file
* "Silabs_License_Agreement.txt" for details. Before using this software for
* any purpose, you must agree to the terms of that agreement.
*
******************************************************************************/

#ifndef DISPLAY_LS013B7DH03_CONFIG_H
#define DISPLAY_LS013B7DH03_CONFIG_H

#include "displayconfigapp.h"

/* Display device name. */
#define SHARP_MEMLCD_DEVICE_NAME   "Sharp LS013B7DH03 #1"

// io expander pins not yet supported
/* LCD and SPI GPIO pin connections on the BRD4154A. */
#define LCD_PORT_SCLK             (gpioPortB)  /* EFM_DISP_SCLK on PC8 */
#define LCD_PIN_SCLK              (11)
#define LCD_PORT_SI               (gpioPortA)  /* EFM_DISP_MOSI on PC6 */
#define LCD_PIN_SI                (0)
#define LCD_PORT_SCS              (gpioPortD)  /* EFM_DISP_CS on PD14 */
#define LCD_PIN_SCS               (13)
// #define LCD_PORT_EXTCOMIN         (ioExpanderPort1)  /* EFM_DISP_COM on PD13 */
// #define LCD_PIN_EXTCOMIN          (3)
// #define LCD_PORT_DISP_PWR         (ioExpanderPort1)  /* EFM_DISP_ENABLE on PD15 */
// #define LCD_PIN_DISP_PWR          (2)

/* PRS settings for polarity inversion extcomin auto toggle.  */
#define LCD_AUTO_TOGGLE_PRS_CH    (4)  /* PRS channel 4.      */
#define LCD_AUTO_TOGGLE_PRS_ROUTELOC()  PRS->ROUTELOC1 = \
  ((PRS->ROUTELOC1 & ~_PRS_ROUTELOC1_CH4LOC_MASK) | PRS_ROUTELOC1_CH4LOC_LOC4)
#define LCD_AUTO_TOGGLE_PRS_ROUTEPEN    PRS_ROUTEPEN_CH4PEN

/*
 * Select how LCD polarity inversion should be handled:
 *
 * If POLARITY_INVERSION_EXTCOMIN is defined, the polarity inversion is armed
 * for every rising edge of the EXTCOMIN pin. The actual polarity inversion is
 * triggered at the next transision of SCS. This mode is recommended because it
 * causes less CPU and SPI load than the alternative mode, see below.
 * If POLARITY_INVERSION_EXTCOMIN is undefined, the polarity inversion is
 * toggled by sending an SPI command. This mode causes more CPU and SPI load
 * than using the EXTCOMIN pin mode.
 */
#define POLARITY_INVERSION_EXTCOMIN

/* Define POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE if you want the PAL
 * (Platform Abstraction Layer interface) to automatically toggle the EXTCOMIN
 *  pin.
 * If the PAL_TIMER_REPEAT function is defined the EXTCOMIN toggling is handled
 * by a timer repeat system, therefore we must undefine
 * POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE;
 */
#ifndef PAL_TIMER_REPEAT_FUNCTION
  #define POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE
#endif

#endif /* DISPLAY_LS013B7DH03_CONFIG_H */
