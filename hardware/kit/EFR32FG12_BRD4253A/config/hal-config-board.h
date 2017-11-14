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

#define BSP_BTL_BUTTON_PIN                            (6)
#define BSP_BTL_BUTTON_PORT                           (gpioPortF)

// [BTL_BUTTON]$

// $[BULBPWM]
// [BULBPWM]$

// $[BULBPWM_COLOR]
// [BULBPWM_COLOR]$

// $[BUTTON]
#define BSP_BUTTON_PRESENT                            (1)

#define BSP_BUTTON0_PIN                               (6)
#define BSP_BUTTON0_PORT                              (gpioPortF)

#define BSP_BUTTON1_PIN                               (7)
#define BSP_BUTTON1_PORT                              (gpioPortF)

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
#define BSP_CLK_HFXO_CTUNE                            (345)
#define BSP_CLK_HFXO_INIT                              CMU_HFXOINIT_DEFAULT
#define BSP_CLK_HFXO_CTUNE_TOKEN                      (0)
// [CMU]$

// $[COEX]
// [COEX]$

// $[CS5463]
// [CS5463]$

// $[CSEN0]
// [CSEN0]$

// $[DCDC]
#define BSP_DCDC_PRESENT                              (1)

#define BSP_DCDC_INIT                                  EMU_DCDCINIT_DEFAULT
// [DCDC]$

// $[EMU]
// [EMU]$

// $[EXTFLASH]
#define BSP_EXTFLASH_CS_PIN                           (4)
#define BSP_EXTFLASH_CS_PORT                          (gpioPortA)

#define BSP_EXTFLASH_USART                            (HAL_SPI_PORT_USART1)
#define BSP_EXTFLASH_INTERNAL                         (0)
#define BSP_EXTFLASH_CLK_PIN                          (8)
#define BSP_EXTFLASH_CLK_PORT                         (gpioPortC)
#define BSP_EXTFLASH_CLK_LOC                          (11)

#define BSP_EXTFLASH_MISO_PIN                         (7)
#define BSP_EXTFLASH_MISO_PORT                        (gpioPortC)
#define BSP_EXTFLASH_MISO_LOC                         (11)

#define BSP_EXTFLASH_MOSI_PIN                         (6)
#define BSP_EXTFLASH_MOSI_PORT                        (gpioPortC)
#define BSP_EXTFLASH_MOSI_LOC                         (11)

// [EXTFLASH]$

// $[EZRADIOPRO]
// [EZRADIOPRO]$

// $[GPIO]
#define PORTIO_GPIO_SWCLKTCK_PIN                      (0)
#define PORTIO_GPIO_SWCLKTCK_PORT                     (gpioPortF)
#define PORTIO_GPIO_DBGROUTE_LOC                      (0)

#define PORTIO_GPIO_SWDIOTMS_PIN                      (1)
#define PORTIO_GPIO_SWDIOTMS_PORT                     (gpioPortF)

#define PORTIO_GPIO_SWV_PIN                           (2)
#define PORTIO_GPIO_SWV_PORT                          (gpioPortF)
#define PORTIO_GPIO_SWV_LOC                           (0)

#define PORTIO_GPIO_TCLK_PIN                          (8)
#define PORTIO_GPIO_TCLK_PORT                         (gpioPortF)
#define PORTIO_GPIO_TCLK_LOC                          (0)

#define PORTIO_GPIO_TD0_PIN                           (9)
#define PORTIO_GPIO_TD0_PORT                          (gpioPortF)
#define PORTIO_GPIO_TD0_LOC                           (0)

#define PORTIO_GPIO_TD1_PIN                           (10)
#define PORTIO_GPIO_TD1_PORT                          (gpioPortF)
#define PORTIO_GPIO_TD1_LOC                           (0)

#define PORTIO_GPIO_TD2_PIN                           (11)
#define PORTIO_GPIO_TD2_PORT                          (gpioPortF)
#define PORTIO_GPIO_TD2_LOC                           (0)

#define PORTIO_GPIO_TD3_PIN                           (12)
#define PORTIO_GPIO_TD3_PORT                          (gpioPortF)
#define PORTIO_GPIO_TD3_LOC                           (0)

#define BSP_TRACE_SWO_PIN                             (2)
#define BSP_TRACE_SWO_PORT                            (gpioPortF)
#define BSP_TRACE_SWO_LOC                             (0)

// [GPIO]$

// $[I2C0]
#define PORTIO_I2C0_SCL_PIN                           (10)
#define PORTIO_I2C0_SCL_PORT                          (gpioPortC)
#define PORTIO_I2C0_SCL_LOC                           (14)

#define PORTIO_I2C0_SDA_PIN                           (11)
#define PORTIO_I2C0_SDA_PORT                          (gpioPortC)
#define PORTIO_I2C0_SDA_LOC                           (16)

#define BSP_I2C0_SDA_PIN                              (11)
#define BSP_I2C0_SDA_PORT                             (gpioPortC)
#define BSP_I2C0_SDA_LOC                              (16)

#define BSP_I2C0_SCL_PIN                              (10)
#define BSP_I2C0_SCL_PORT                             (gpioPortC)
#define BSP_I2C0_SCL_LOC                              (14)

// [I2C0]$

// $[I2C1]
// [I2C1]$

// $[I2CSENSOR]

#define BSP_I2CSENSOR_ENABLE_PIN                      (10)
#define BSP_I2CSENSOR_ENABLE_PORT                     (gpioPortB)

#define BSP_I2CSENSOR_PERIPHERAL                      (HAL_I2C_PORT_I2C0)
#define BSP_I2CSENSOR_SDA_PIN                         (11)
#define BSP_I2CSENSOR_SDA_PORT                        (gpioPortC)
#define BSP_I2CSENSOR_SDA_LOC                         (16)

#define BSP_I2CSENSOR_SCL_PIN                         (10)
#define BSP_I2CSENSOR_SCL_PORT                        (gpioPortC)
#define BSP_I2CSENSOR_SCL_LOC                         (14)

// [I2CSENSOR]$

// $[IDAC0]
// [IDAC0]$

// $[IOEXP]
// [IOEXP]$

// $[LED]
#define BSP_LED_PRESENT                               (1)

#define BSP_LED0_PIN                                  (4)
#define BSP_LED0_PORT                                 (gpioPortF)

#define BSP_LED1_PIN                                  (5)
#define BSP_LED1_PORT                                 (gpioPortF)

#define BSP_LED_COUNT                                 (2)
#define BSP_LED_INIT                                  { { BSP_LED0_PORT, BSP_LED0_PIN }, { BSP_LED1_PORT, BSP_LED1_PIN } }
// [LED]$

// $[LESENSE]
// [LESENSE]$

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

// $[PCNT1]
// [PCNT1]$

// $[PCNT2]
// [PCNT2]$

// $[PORTIO]
// [PORTIO]$

// $[PRS]
#define PORTIO_PRS_CH4_PIN                            (13)
#define PORTIO_PRS_CH4_PORT                           (gpioPortD)
#define PORTIO_PRS_CH4_LOC                            (4)

// [PRS]$

// $[PTI]
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
#define BSP_SERIAL_APP_CTS_PIN                        (2)
#define BSP_SERIAL_APP_CTS_PORT                       (gpioPortA)
#define BSP_SERIAL_APP_CTS_LOC                        (30)

#define BSP_SERIAL_APP_RX_PIN                         (1)
#define BSP_SERIAL_APP_RX_PORT                        (gpioPortA)
#define BSP_SERIAL_APP_RX_LOC                         (0)

#define BSP_SERIAL_APP_TX_PIN                         (0)
#define BSP_SERIAL_APP_TX_PORT                        (gpioPortA)
#define BSP_SERIAL_APP_TX_LOC                         (0)

#define BSP_SERIAL_APP_RTS_PIN                        (3)
#define BSP_SERIAL_APP_RTS_PORT                       (gpioPortA)
#define BSP_SERIAL_APP_RTS_LOC                        (30)

// [SERIAL]$

// $[SPIDISPLAY]

#define BSP_SPIDISPLAY_ENABLE_PIN                     (15)
#define BSP_SPIDISPLAY_ENABLE_PORT                    (gpioPortD)

#define BSP_SPIDISPLAY_CS_PIN                         (14)
#define BSP_SPIDISPLAY_CS_PORT                        (gpioPortD)

#define BSP_SPIDISPLAY_EXTCOMIN_PIN                   (13)
#define BSP_SPIDISPLAY_EXTCOMIN_PORT                  (gpioPortD)
#define BSP_SPIDISPLAY_EXTCOMIN_LOC                   (4)

#define BSP_SPIDISPLAY_USART                          (HAL_SPI_PORT_USART1)
#define BSP_SPIDISPLAY_DISPLAY                        (HAL_DISPLAY_SHARP_LS013B7DH03)
#define BSP_SPIDISPLAY_EXTCOMIN_CHANNEL               (4)
#define BSP_SPIDISPLAY_CLK_PIN                        (8)
#define BSP_SPIDISPLAY_CLK_PORT                       (gpioPortC)
#define BSP_SPIDISPLAY_CLK_LOC                        (11)

#define BSP_SPIDISPLAY_MISO_PIN                       (7)
#define BSP_SPIDISPLAY_MISO_PORT                      (gpioPortC)
#define BSP_SPIDISPLAY_MISO_LOC                       (11)

#define BSP_SPIDISPLAY_MOSI_PIN                       (6)
#define BSP_SPIDISPLAY_MOSI_PORT                      (gpioPortC)
#define BSP_SPIDISPLAY_MOSI_LOC                       (11)

// [SPIDISPLAY]$

// $[SPINCP]
#define BSP_SPINCP_NWAKE_PIN                          (11)
#define BSP_SPINCP_NWAKE_PORT                         (gpioPortD)

#define BSP_SPINCP_NHOSTINT_PIN                       (10)
#define BSP_SPINCP_NHOSTINT_PORT                      (gpioPortD)

#define BSP_SPINCP_USART_PORT                         (HAL_SPI_PORT_USART2)
#define BSP_SPINCP_CS_PIN                             (9)
#define BSP_SPINCP_CS_PORT                            (gpioPortA)
#define BSP_SPINCP_CS_LOC                             (1)

#define BSP_SPINCP_CLK_PIN                            (8)
#define BSP_SPINCP_CLK_PORT                           (gpioPortA)
#define BSP_SPINCP_CLK_LOC                            (1)

#define BSP_SPINCP_MISO_PIN                           (7)
#define BSP_SPINCP_MISO_PORT                          (gpioPortA)
#define BSP_SPINCP_MISO_LOC                           (1)

#define BSP_SPINCP_MOSI_PIN                           (6)
#define BSP_SPINCP_MOSI_PORT                          (gpioPortA)
#define BSP_SPINCP_MOSI_LOC                           (1)

// [SPINCP]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[UARTNCP]
#define BSP_UARTNCP_USART_PORT                        (HAL_SERIAL_PORT_USART0)
#define BSP_UARTNCP_CTS_PIN                           (2)
#define BSP_UARTNCP_CTS_PORT                          (gpioPortA)
#define BSP_UARTNCP_CTS_LOC                           (30)

#define BSP_UARTNCP_RX_PIN                            (1)
#define BSP_UARTNCP_RX_PORT                           (gpioPortA)
#define BSP_UARTNCP_RX_LOC                            (0)

#define BSP_UARTNCP_TX_PIN                            (0)
#define BSP_UARTNCP_TX_PORT                           (gpioPortA)
#define BSP_UARTNCP_TX_LOC                            (0)

#define BSP_UARTNCP_RTS_PIN                           (3)
#define BSP_UARTNCP_RTS_PORT                          (gpioPortA)
#define BSP_UARTNCP_RTS_LOC                           (30)

// [UARTNCP]$

// $[USART0]
#define PORTIO_USART0_CTS_PIN                         (2)
#define PORTIO_USART0_CTS_PORT                        (gpioPortA)
#define PORTIO_USART0_CTS_LOC                         (30)

#define PORTIO_USART0_RTS_PIN                         (3)
#define PORTIO_USART0_RTS_PORT                        (gpioPortA)
#define PORTIO_USART0_RTS_LOC                         (30)

#define PORTIO_USART0_RX_PIN                          (1)
#define PORTIO_USART0_RX_PORT                         (gpioPortA)
#define PORTIO_USART0_RX_LOC                          (0)

#define PORTIO_USART0_TX_PIN                          (0)
#define PORTIO_USART0_TX_PORT                         (gpioPortA)
#define PORTIO_USART0_TX_LOC                          (0)

#define BSP_USART0_CTS_PIN                            (2)
#define BSP_USART0_CTS_PORT                           (gpioPortA)
#define BSP_USART0_CTS_LOC                            (30)

#define BSP_USART0_RX_PIN                             (1)
#define BSP_USART0_RX_PORT                            (gpioPortA)
#define BSP_USART0_RX_LOC                             (0)

#define BSP_USART0_TX_PIN                             (0)
#define BSP_USART0_TX_PORT                            (gpioPortA)
#define BSP_USART0_TX_LOC                             (0)

#define BSP_USART0_RTS_PIN                            (3)
#define BSP_USART0_RTS_PORT                           (gpioPortA)
#define BSP_USART0_RTS_LOC                            (30)

// [USART0]$

// $[USART1]
#define PORTIO_USART1_CLK_PIN                         (8)
#define PORTIO_USART1_CLK_PORT                        (gpioPortC)
#define PORTIO_USART1_CLK_LOC                         (11)

#define PORTIO_USART1_CS_PIN                          (9)
#define PORTIO_USART1_CS_PORT                         (gpioPortC)
#define PORTIO_USART1_CS_LOC                          (11)

#define PORTIO_USART1_RX_PIN                          (7)
#define PORTIO_USART1_RX_PORT                         (gpioPortC)
#define PORTIO_USART1_RX_LOC                          (11)

#define PORTIO_USART1_TX_PIN                          (6)
#define PORTIO_USART1_TX_PORT                         (gpioPortC)
#define PORTIO_USART1_TX_LOC                          (11)

#define BSP_USART1_CS_PIN                             (9)
#define BSP_USART1_CS_PORT                            (gpioPortC)
#define BSP_USART1_CS_LOC                             (11)

#define BSP_USART1_CLK_PIN                            (8)
#define BSP_USART1_CLK_PORT                           (gpioPortC)
#define BSP_USART1_CLK_LOC                            (11)

#define BSP_USART1_MISO_PIN                           (7)
#define BSP_USART1_MISO_PORT                          (gpioPortC)
#define BSP_USART1_MISO_LOC                           (11)

#define BSP_USART1_MOSI_PIN                           (6)
#define BSP_USART1_MOSI_PORT                          (gpioPortC)
#define BSP_USART1_MOSI_LOC                           (11)

// [USART1]$

// $[USART2]
#define PORTIO_USART2_CLK_PIN                         (8)
#define PORTIO_USART2_CLK_PORT                        (gpioPortA)
#define PORTIO_USART2_CLK_LOC                         (1)

#define PORTIO_USART2_CS_PIN                          (9)
#define PORTIO_USART2_CS_PORT                         (gpioPortA)
#define PORTIO_USART2_CS_LOC                          (1)

#define PORTIO_USART2_RX_PIN                          (7)
#define PORTIO_USART2_RX_PORT                         (gpioPortA)
#define PORTIO_USART2_RX_LOC                          (1)

#define PORTIO_USART2_TX_PIN                          (6)
#define PORTIO_USART2_TX_PORT                         (gpioPortA)
#define PORTIO_USART2_TX_LOC                          (1)

#define BSP_USART2_CS_PIN                             (9)
#define BSP_USART2_CS_PORT                            (gpioPortA)
#define BSP_USART2_CS_LOC                             (1)

#define BSP_USART2_CLK_PIN                            (8)
#define BSP_USART2_CLK_PORT                           (gpioPortA)
#define BSP_USART2_CLK_LOC                            (1)

#define BSP_USART2_MISO_PIN                           (7)
#define BSP_USART2_MISO_PORT                          (gpioPortA)
#define BSP_USART2_MISO_LOC                           (1)

#define BSP_USART2_MOSI_PIN                           (6)
#define BSP_USART2_MOSI_PORT                          (gpioPortA)
#define BSP_USART2_MOSI_LOC                           (1)

// [USART2]$

// $[USART3]
#define PORTIO_USART3_CTS_PIN                         (8)
#define PORTIO_USART3_CTS_PORT                        (gpioPortD)
#define PORTIO_USART3_CTS_LOC                         (28)

#define PORTIO_USART3_RTS_PIN                         (9)
#define PORTIO_USART3_RTS_PORT                        (gpioPortD)
#define PORTIO_USART3_RTS_LOC                         (28)

#define PORTIO_USART3_RX_PIN                          (7)
#define PORTIO_USART3_RX_PORT                         (gpioPortB)
#define PORTIO_USART3_RX_LOC                          (10)

#define PORTIO_USART3_TX_PIN                          (6)
#define PORTIO_USART3_TX_PORT                         (gpioPortB)
#define PORTIO_USART3_TX_LOC                          (10)

#define BSP_USART3_CTS_PIN                            (8)
#define BSP_USART3_CTS_PORT                           (gpioPortD)
#define BSP_USART3_CTS_LOC                            (28)

#define BSP_USART3_RX_PIN                             (7)
#define BSP_USART3_RX_PORT                            (gpioPortB)
#define BSP_USART3_RX_LOC                             (10)

#define BSP_USART3_TX_PIN                             (6)
#define BSP_USART3_TX_PORT                            (gpioPortB)
#define BSP_USART3_TX_LOC                             (10)

#define BSP_USART3_RTS_PIN                            (9)
#define BSP_USART3_RTS_PORT                           (gpioPortD)
#define BSP_USART3_RTS_LOC                            (28)

// [USART3]$

// $[VCOM]

#define BSP_VCOM_ENABLE_PIN                           (5)
#define BSP_VCOM_ENABLE_PORT                          (gpioPortA)

// [VCOM]$

// $[VDAC0]
// [VDAC0]$

// $[VUART]
// [VUART]$

// $[WDOG]
// [WDOG]$

// $[WTIMER0]
// [WTIMER0]$

// $[WTIMER1]
// [WTIMER1]$

#endif /* HAL_CONFIG_BOARD_H */
