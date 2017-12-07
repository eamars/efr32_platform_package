#ifndef HAL_CONFIG_BOARD_H
#define HAL_CONFIG_BOARD_H

#include "em_device.h"
#include "hal-config-types.h"

// This file is auto-generated by Hardware Configurator in Simplicity Studio.
// Any content between $[ and ]$ will be replaced whenever the file is regenerated.
// Content outside these regions will be preserved.

// $[ACMP0]
// [ACMP0]$

// $[ACMP1]
// [ACMP1]$

// $[ADC0]
// [ADC0]$

// $[ANTDIV]
// [ANTDIV]$

// $[BATTERYMON]
// [BATTERYMON]$

// $[BTL_BUTTON]

#define BSP_BTL_BUTTON_PIN                            (14)
#define BSP_BTL_BUTTON_PORT                           (gpioPortD)

// [BTL_BUTTON]$

// $[BULBPWM]
// [BULBPWM]$

// $[BULBPWM_COLOR]
// [BULBPWM_COLOR]$

// $[BUTTON]
#define BSP_BUTTON_PRESENT                            (1)

#define BSP_BUTTON0_PIN                               (14)
#define BSP_BUTTON0_PORT                              (gpioPortD)

#define BSP_BUTTON1_PIN                               (15)
#define BSP_BUTTON1_PORT                              (gpioPortD)

#define BSP_BUTTON_COUNT                              (2)
#define BSP_BUTTON_INIT                               { { BSP_BUTTON0_PORT, BSP_BUTTON0_PIN }, { BSP_BUTTON1_PORT, BSP_BUTTON1_PIN } }
#define BSP_BUTTON_GPIO_DOUT                          (HAL_GPIO_DOUT_LOW)
#define BSP_BUTTON_GPIO_MODE                          (HAL_GPIO_MODE_INPUT)
// [BUTTON]$

// $[CMU]
#define BSP_CLK_LFXO_PRESENT                          (1)
#define BSP_CLK_HFXO_PRESENT                          (1)
#define BSP_CLK_LFXO_INIT                              CMU_LFXOINIT_DEFAULT
#define BSP_CLK_LFXO_CTUNE                            (0)
#define BSP_CLK_LFXO_FREQ                             (32768)
#define BSP_CLK_HFXO_FREQ                             (38400000)
#define BSP_CLK_HFXO_CTUNE                            (328)
#define BSP_CLK_HFXO_INIT                              CMU_HFXOINIT_DEFAULT
#define BSP_CLK_HFXO_CTUNE_TOKEN                      (0)
// [CMU]$

// $[COEX]
// [COEX]$

// $[CS5463]
// [CS5463]$

// $[DCDC]
#define BSP_DCDC_PRESENT                              (1)

#define BSP_DCDC_INIT                                  EMU_DCDCINIT_DEFAULT
// [DCDC]$

// $[EMU]
// [EMU]$

// $[EXTFLASH]
#define BSP_EXTFLASH_USART                            (HAL_SPI_PORT_USART1)
#define BSP_EXTFLASH_INTERNAL                         (1)
#define BSP_EXTFLASH_WP_PIN                           (7)
#define BSP_EXTFLASH_WP_PORT                          (gpioPortF)
#define BSP_EXTFLASH_CS_PIN                           (6)
#define BSP_EXTFLASH_CS_PORT                          (gpioPortC)
#define BSP_EXTFLASH_MISO_PIN                         (7)
#define BSP_EXTFLASH_MISO_PORT                        (gpioPortC)
#define BSP_EXTFLASH_MISO_LOC                         (11)
#define BSP_EXTFLASH_MOSI_PIN                         (6)
#define BSP_EXTFLASH_MOSI_PORT                        (gpioPortF)
#define BSP_EXTFLASH_MOSI_LOC                         (30)
#define BSP_EXTFLASH_CLK_PIN                          (9)
#define BSP_EXTFLASH_CLK_PORT                         (gpioPortC)
#define BSP_EXTFLASH_CLK_LOC                          (12)
#define BSP_EXTFLASH_HOLD_PIN                         (8)
#define BSP_EXTFLASH_HOLD_PORT                        (gpioPortC)
// [EXTFLASH]$

// $[EZRADIOPRO]
// [EZRADIOPRO]$

// $[GPIO]
// [GPIO]$

// $[I2C0]
#define PORTIO_I2C0_SCL_PIN                           (11)
#define PORTIO_I2C0_SCL_PORT                          (gpioPortC)
#define PORTIO_I2C0_SCL_LOC                           (15)

#define PORTIO_I2C0_SDA_PIN                           (10)
#define PORTIO_I2C0_SDA_PORT                          (gpioPortC)
#define PORTIO_I2C0_SDA_LOC                           (15)

#define BSP_I2C0_SDA_PIN                              (10)
#define BSP_I2C0_SDA_PORT                             (gpioPortC)
#define BSP_I2C0_SDA_LOC                              (15)

#define BSP_I2C0_SCL_PIN                              (11)
#define BSP_I2C0_SCL_PORT                             (gpioPortC)
#define BSP_I2C0_SCL_LOC                              (15)

// [I2C0]$

// $[I2CSENSOR]

#define BSP_I2CSENSOR_PERIPHERAL                      (HAL_I2C_PORT_I2C0)
#define BSP_I2CSENSOR_SDA_PIN                         (10)
#define BSP_I2CSENSOR_SDA_PORT                        (gpioPortC)
#define BSP_I2CSENSOR_SDA_LOC                         (15)

#define BSP_I2CSENSOR_SCL_PIN                         (11)
#define BSP_I2CSENSOR_SCL_PORT                        (gpioPortC)
#define BSP_I2CSENSOR_SCL_LOC                         (15)

// [I2CSENSOR]$

// $[IDAC0]
// [IDAC0]$

// $[IOEXP]

#define BSP_IOEXP_WAKE_PIN                            (3)
#define BSP_IOEXP_WAKE_PORT                           (gpioPortF)

#define BSP_IOEXP_PERIPHERAL                          (HAL_I2C_PORT_I2C0)
#define BSP_IOEXP_SDA_PIN                             (10)
#define BSP_IOEXP_SDA_PORT                            (gpioPortC)
#define BSP_IOEXP_SDA_LOC                             (15)

#define BSP_IOEXP_SCL_PIN                             (11)
#define BSP_IOEXP_SCL_PORT                            (gpioPortC)
#define BSP_IOEXP_SCL_LOC                             (15)

// [IOEXP]$

// $[LED]
#define BSP_LED_PRESENT                               (1)

#define BSP_LED0_PIN                                  (14)
#define BSP_LED0_PORT                                 (gpioPortD)

#define BSP_LED1_PIN                                  (15)
#define BSP_LED1_PORT                                 (gpioPortD)

#define BSP_LED_COUNT                                 (2)
#define BSP_LED_INIT                                  { { BSP_LED0_PORT, BSP_LED0_PIN }, { BSP_LED1_PORT, BSP_LED1_PIN } }
// [LED]$

// $[LETIMER0]
// [LETIMER0]$

// $[LEUART0]
// [LEUART0]$

// $[LFXO]
// [LFXO]$

// $[LNA]
// [LNA]$

// $[PA]
// [PA]$

// $[PCNT0]
// [PCNT0]$

// $[PORTIO]
// [PORTIO]$

// $[PRS]
// [PRS]$

// $[PTI]
#define PORTIO_PTI_DCLK_PIN                           (11)
#define PORTIO_PTI_DCLK_PORT                          (gpioPortB)
#define PORTIO_PTI_DCLK_LOC                           (6)

#define PORTIO_PTI_DFRAME_PIN                         (13)
#define PORTIO_PTI_DFRAME_PORT                        (gpioPortB)
#define PORTIO_PTI_DFRAME_LOC                         (6)

#define PORTIO_PTI_DOUT_PIN                           (12)
#define PORTIO_PTI_DOUT_PORT                          (gpioPortB)
#define PORTIO_PTI_DOUT_LOC                           (6)

#define BSP_PTI_DFRAME_PIN                            (13)
#define BSP_PTI_DFRAME_PORT                           (gpioPortB)
#define BSP_PTI_DFRAME_LOC                            (6)

#define BSP_PTI_DOUT_PIN                              (12)
#define BSP_PTI_DOUT_PORT                             (gpioPortB)
#define BSP_PTI_DOUT_LOC                              (6)

// [PTI]$

// $[PYD1698]
// [PYD1698]$

// $[SERIAL]
#define BSP_SERIAL_APP_PORT                           (HAL_SERIAL_PORT_USART0)
#define BSP_SERIAL_APP_CTS_PIN                        (11)
#define BSP_SERIAL_APP_CTS_PORT                       (gpioPortB)
#define BSP_SERIAL_APP_CTS_LOC                        (2)

#define BSP_SERIAL_APP_RX_PIN                         (1)
#define BSP_SERIAL_APP_RX_PORT                        (gpioPortA)
#define BSP_SERIAL_APP_RX_LOC                         (0)

#define BSP_SERIAL_APP_TX_PIN                         (0)
#define BSP_SERIAL_APP_TX_PORT                        (gpioPortA)
#define BSP_SERIAL_APP_TX_LOC                         (0)

#define BSP_SERIAL_APP_RTS_PIN                        (13)
#define BSP_SERIAL_APP_RTS_PORT                       (gpioPortD)
#define BSP_SERIAL_APP_RTS_LOC                        (16)

// [SERIAL]$

// $[SPIDISPLAY]

#define BSP_SPIDISPLAY_CS_PIN                         (13)
#define BSP_SPIDISPLAY_CS_PORT                        (gpioPortD)

#define BSP_SPIDISPLAY_USART                          (HAL_SPI_PORT_USART1)
#define BSP_SPIDISPLAY_DISPLAY                        (HAL_DISPLAY_SHARP_LS013B7DH03)
#define BSP_SPIDISPLAY_CLK_PIN                        (11)
#define BSP_SPIDISPLAY_CLK_PORT                       (gpioPortB)
#define BSP_SPIDISPLAY_CLK_LOC                        (4)

#define BSP_SPIDISPLAY_MISO_PIN                       (1)
#define BSP_SPIDISPLAY_MISO_PORT                      (gpioPortA)
#define BSP_SPIDISPLAY_MISO_LOC                       (0)

#define BSP_SPIDISPLAY_MOSI_PIN                       (0)
#define BSP_SPIDISPLAY_MOSI_PORT                      (gpioPortA)
#define BSP_SPIDISPLAY_MOSI_LOC                       (0)

// [SPIDISPLAY]$

// $[SPINCP]
#define BSP_SPINCP_NWAKE_PIN                          (15)
#define BSP_SPINCP_NWAKE_PORT                         (gpioPortD)

#define BSP_SPINCP_NHOSTINT_PIN                       (14)
#define BSP_SPINCP_NHOSTINT_PORT                      (gpioPortD)

#define BSP_SPINCP_USART_PORT                         (HAL_SPI_PORT_USART1)
#define BSP_SPINCP_CS_PIN                             (13)
#define BSP_SPINCP_CS_PORT                            (gpioPortD)
#define BSP_SPINCP_CS_LOC                             (18)

#define BSP_SPINCP_CLK_PIN                            (11)
#define BSP_SPINCP_CLK_PORT                           (gpioPortB)
#define BSP_SPINCP_CLK_LOC                            (4)

#define BSP_SPINCP_MISO_PIN                           (1)
#define BSP_SPINCP_MISO_PORT                          (gpioPortA)
#define BSP_SPINCP_MISO_LOC                           (0)

#define BSP_SPINCP_MOSI_PIN                           (0)
#define BSP_SPINCP_MOSI_PORT                          (gpioPortA)
#define BSP_SPINCP_MOSI_LOC                           (0)

// [SPINCP]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[UARTNCP]
#define BSP_UARTNCP_USART_PORT                        (HAL_SERIAL_PORT_USART0)
#define BSP_UARTNCP_CTS_PIN                           (11)
#define BSP_UARTNCP_CTS_PORT                          (gpioPortB)
#define BSP_UARTNCP_CTS_LOC                           (2)

#define BSP_UARTNCP_RX_PIN                            (1)
#define BSP_UARTNCP_RX_PORT                           (gpioPortA)
#define BSP_UARTNCP_RX_LOC                            (0)

#define BSP_UARTNCP_TX_PIN                            (0)
#define BSP_UARTNCP_TX_PORT                           (gpioPortA)
#define BSP_UARTNCP_TX_LOC                            (0)

#define BSP_UARTNCP_RTS_PIN                           (13)
#define BSP_UARTNCP_RTS_PORT                          (gpioPortD)
#define BSP_UARTNCP_RTS_LOC                           (16)

// [UARTNCP]$

// $[USART0]
#define PORTIO_USART0_CLK_PIN                         (11)
#define PORTIO_USART0_CLK_PORT                        (gpioPortB)
#define PORTIO_USART0_CLK_LOC                         (4)

#define PORTIO_USART0_CS_PIN                          (13)
#define PORTIO_USART0_CS_PORT                         (gpioPortD)
#define PORTIO_USART0_CS_LOC                          (18)

#define PORTIO_USART0_CTS_PIN                         (11)
#define PORTIO_USART0_CTS_PORT                        (gpioPortB)
#define PORTIO_USART0_CTS_LOC                         (2)

#define PORTIO_USART0_RTS_PIN                         (13)
#define PORTIO_USART0_RTS_PORT                        (gpioPortD)
#define PORTIO_USART0_RTS_LOC                         (16)

#define PORTIO_USART0_RX_PIN                          (1)
#define PORTIO_USART0_RX_PORT                         (gpioPortA)
#define PORTIO_USART0_RX_LOC                          (0)

#define PORTIO_USART0_TX_PIN                          (0)
#define PORTIO_USART0_TX_PORT                         (gpioPortA)
#define PORTIO_USART0_TX_LOC                          (0)

#define BSP_USART0_CTS_PIN                            (11)
#define BSP_USART0_CTS_PORT                           (gpioPortB)
#define BSP_USART0_CTS_LOC                            (2)

#define BSP_USART0_RX_PIN                             (1)
#define BSP_USART0_RX_PORT                            (gpioPortA)
#define BSP_USART0_RX_LOC                             (0)

#define BSP_USART0_TX_PIN                             (0)
#define BSP_USART0_TX_PORT                            (gpioPortA)
#define BSP_USART0_TX_LOC                             (0)

#define BSP_USART0_RTS_PIN                            (13)
#define BSP_USART0_RTS_PORT                           (gpioPortD)
#define BSP_USART0_RTS_LOC                            (16)

// [USART0]$

// $[USART1]
#define PORTIO_USART1_CLK_PIN                         (11)
#define PORTIO_USART1_CLK_PORT                        (gpioPortB)
#define PORTIO_USART1_CLK_LOC                         (4)

#define PORTIO_USART1_CS_PIN                          (13)
#define PORTIO_USART1_CS_PORT                         (gpioPortD)
#define PORTIO_USART1_CS_LOC                          (18)

#define PORTIO_USART1_RX_PIN                          (1)
#define PORTIO_USART1_RX_PORT                         (gpioPortA)
#define PORTIO_USART1_RX_LOC                          (0)

#define PORTIO_USART1_TX_PIN                          (0)
#define PORTIO_USART1_TX_PORT                         (gpioPortA)
#define PORTIO_USART1_TX_LOC                          (0)

#define BSP_USART1_CS_PIN                             (13)
#define BSP_USART1_CS_PORT                            (gpioPortD)
#define BSP_USART1_CS_LOC                             (18)

#define BSP_USART1_CLK_PIN                            (11)
#define BSP_USART1_CLK_PORT                           (gpioPortB)
#define BSP_USART1_CLK_LOC                            (4)

#define BSP_USART1_MISO_PIN                           (1)
#define BSP_USART1_MISO_PORT                          (gpioPortA)
#define BSP_USART1_MISO_LOC                           (0)

#define BSP_USART1_MOSI_PIN                           (0)
#define BSP_USART1_MOSI_PORT                          (gpioPortA)
#define BSP_USART1_MOSI_LOC                           (0)

// [USART1]$

// $[VCOM]
// [VCOM]$

// $[VUART]
// [VUART]$

// $[WDOG]
// [WDOG]$

#endif /* HAL_CONFIG_BOARD_H */