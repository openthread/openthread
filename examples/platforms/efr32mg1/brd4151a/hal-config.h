#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "em_device.h"
#include "hal-config-types.h"

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

#define BSP_BTL_BUTTON_PIN                            (6U)
#define BSP_BTL_BUTTON_PORT                           (gpioPortF)

// [BTL_BUTTON]$

// $[BULBPWM]
// [BULBPWM]$

// $[BULBPWM_COLOR]
// [BULBPWM_COLOR]$

// $[BUTTON]
#define BSP_BUTTON_PRESENT                            (1)

#define BSP_BUTTON0_PIN                               (6U)
#define BSP_BUTTON0_PORT                              (gpioPortF)

#define BSP_BUTTON1_PIN                               (7U)
#define BSP_BUTTON1_PORT                              (gpioPortF)

#define BSP_BUTTON_COUNT                              (2U)
#define BSP_BUTTON_INIT                               { { BSP_BUTTON0_PORT, BSP_BUTTON0_PIN }, { BSP_BUTTON1_PORT, BSP_BUTTON1_PIN } }
#define BSP_BUTTON_GPIO_DOUT                          (HAL_GPIO_DOUT_LOW)
#define BSP_BUTTON_GPIO_MODE                          (HAL_GPIO_MODE_INPUT)
// [BUTTON]$

// $[CMU]
#define HAL_CLK_HFCLK_SOURCE (HAL_CLK_HFCLK_SOURCE_HFXO)
#define HAL_CLK_LFECLK_SOURCE (HAL_CLK_LFCLK_SOURCE_LFRCO)
#define HAL_CLK_LFBCLK_SOURCE (HAL_CLK_LFCLK_SOURCE_LFRCO)
#define BSP_CLK_LFXO_PRESENT (1)
#define BSP_CLK_HFXO_PRESENT (1)
#define BSP_CLK_LFXO_INIT CMU_LFXOINIT_DEFAULT
#define BSP_CLK_LFXO_CTUNE (0)
#define BSP_CLK_LFXO_FREQ (32768U)
#define HAL_CLK_LFACLK_SOURCE (HAL_CLK_LFCLK_SOURCE_LFRCO)
#define BSP_CLK_HFXO_FREQ (38400000UL)
#define BSP_CLK_HFXO_CTUNE (338)
#define BSP_CLK_HFXO_INIT CMU_HFXOINIT_DEFAULT
#define BSP_CLK_HFXO_CTUNE_TOKEN (0)
#define HAL_CLK_HFXO_AUTOSTART (HAL_CLK_HFXO_AUTOSTART_NONE)
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
// [EXTFLASH]$

// $[EZRADIOPRO]
// [EZRADIOPRO]$

// $[FEM]
// [FEM]$

// $[GPIO]
// [GPIO]$

// $[I2C0]
// [I2C0]$

// $[I2CSENSOR]

// [I2CSENSOR]$

// $[IDAC0]
// [IDAC0]$

// $[IOEXP]
// [IOEXP]$

// $[LED]
#define BSP_LED_PRESENT                               (1)

#define BSP_LED0_PIN                                  (4U)
#define BSP_LED0_PORT                                 (gpioPortF)

#define BSP_LED1_PIN                                  (5U)
#define BSP_LED1_PORT                                 (gpioPortF)

#define BSP_LED_COUNT                                 (2U)
#define BSP_LED_INIT                                  { { BSP_LED0_PORT, BSP_LED0_PIN }, { BSP_LED1_PORT, BSP_LED1_PIN } }
#define BSP_LED_POLARITY                              (1)
// [LED]$

// $[LETIMER0]
// [LETIMER0]$

// $[LEUART0]
// [LEUART0]$

// $[LFXO]
// [LFXO]$

// $[MODEM]
// [MODEM]$

// $[PA]

#define HAL_PA_ENABLE (1)

#define HAL_PA_RAMP (10)
#define HAL_PA_2P4_LOWPOWER (0)
#define HAL_PA_POWER (252)
#define HAL_PA_VOLTAGE (3300)
#define HAL_PA_CURVE_HEADER "pa_curves_efr32.h"// [PA]$

// $[PCNT0]
// [PCNT0]$

// $[PORTIO]
// [PORTIO]$

// $[PRS]
#define PORTIO_PRS_CH4_PIN                            (13U)
#define PORTIO_PRS_CH4_PORT                           (gpioPortD)
#define PORTIO_PRS_CH4_LOC                            (4U)

// [PRS]$

// $[PTI]
#define PORTIO_PTI_DCLK_PIN                           (11U)
#define PORTIO_PTI_DCLK_PORT                          (gpioPortB)
#define PORTIO_PTI_DCLK_LOC                           (6U)

#define PORTIO_PTI_DFRAME_PIN                         (13U)
#define PORTIO_PTI_DFRAME_PORT                        (gpioPortB)
#define PORTIO_PTI_DFRAME_LOC                         (6U)

#define PORTIO_PTI_DOUT_PIN                           (12U)
#define PORTIO_PTI_DOUT_PORT                          (gpioPortB)
#define PORTIO_PTI_DOUT_LOC                           (6U)

#define BSP_PTI_DFRAME_PIN                            (13U)
#define BSP_PTI_DFRAME_PORT                           (gpioPortB)
#define BSP_PTI_DFRAME_LOC                            (6U)

#define BSP_PTI_DOUT_PIN                              (12U)
#define BSP_PTI_DOUT_PORT                             (gpioPortB)
#define BSP_PTI_DOUT_LOC                              (6U)

// [PTI]$

// $[PYD1698]
// [PYD1698]$

// $[SERIAL]
#define HAL_SERIAL_USART0_ENABLE (0)
#define HAL_SERIAL_LEUART0_ENABLE (0)
#define HAL_SERIAL_USART1_ENABLE (0)
#define HAL_SERIAL_USART2_ENABLE (0)
#define HAL_SERIAL_USART3_ENABLE (0)
#define HAL_SERIAL_RXWAKE_ENABLE (0)
#define BSP_SERIAL_APP_TX_PIN                         (10U)
#define BSP_SERIAL_APP_TX_PORT                        (gpioPortD)
#define BSP_SERIAL_APP_TX_LOC                         (18U)

#define BSP_SERIAL_APP_RX_PIN                         (11U)
#define BSP_SERIAL_APP_RX_PORT                        (gpioPortD)
#define BSP_SERIAL_APP_RX_LOC                         (18U)

#define BSP_SERIAL_APP_CTS_PIN                        (2U)
#define BSP_SERIAL_APP_CTS_PORT                       (gpioPortA)
#define BSP_SERIAL_APP_CTS_LOC                        (30U)

#define BSP_SERIAL_APP_RTS_PIN                        (3U)
#define BSP_SERIAL_APP_RTS_PORT                       (gpioPortA)
#define BSP_SERIAL_APP_RTS_LOC                        (30U)

#define HAL_SERIAL_APP_RX_QUEUE_SIZE (128)
#define HAL_SERIAL_APP_BAUD_RATE (115200)
#define HAL_SERIAL_APP_RXSTOP (16)
#define HAL_SERIAL_APP_RXSTART (16)
#define HAL_SERIAL_APP_TX_QUEUE_SIZE (128)
#define HAL_SERIAL_APP_FLOW_CONTROL (HAL_USART_FLOW_CONTROL_NONE)
// [SERIAL]$

// $[SPIDISPLAY]
// [SPIDISPLAY]$

// $[SPINCP]
// [SPINCP]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[UARTNCP]
// [UARTNCP]$

// $[USART0]
#define PORTIO_USART0_CTS_PIN                         (2U)
#define PORTIO_USART0_CTS_PORT                        (gpioPortA)
#define PORTIO_USART0_CTS_LOC                         (30U)

#define PORTIO_USART0_RTS_PIN                         (3U)
#define PORTIO_USART0_RTS_PORT                        (gpioPortA)
#define PORTIO_USART0_RTS_LOC                         (30U)

#define PORTIO_USART0_RX_PIN                          (11U)
#define PORTIO_USART0_RX_PORT                         (gpioPortD)
#define PORTIO_USART0_RX_LOC                          (18U)

#define PORTIO_USART0_TX_PIN                          (10U)
#define PORTIO_USART0_TX_PORT                         (gpioPortD)
#define PORTIO_USART0_TX_LOC                          (18U)

#define HAL_USART0_ENABLE (1)

#define BSP_USART0_TX_PIN                             (10U)
#define BSP_USART0_TX_PORT                            (gpioPortD)
#define BSP_USART0_TX_LOC                             (18U)

#define BSP_USART0_RX_PIN                             (11U)
#define BSP_USART0_RX_PORT                            (gpioPortD)
#define BSP_USART0_RX_LOC                             (18U)

#define BSP_USART0_CTS_PIN                            (2U)
#define BSP_USART0_CTS_PORT                           (gpioPortA)
#define BSP_USART0_CTS_LOC                            (30U)

#define BSP_USART0_RTS_PIN                            (3U)
#define BSP_USART0_RTS_PORT                           (gpioPortA)
#define BSP_USART0_RTS_LOC                            (30U)

#define HAL_USART0_RX_QUEUE_SIZE (128)
#define HAL_USART0_BAUD_RATE (115200)
#define HAL_USART0_RXSTOP (16)
#define HAL_USART0_RXSTART (16)
#define HAL_USART0_TX_QUEUE_SIZE (128)
#define HAL_USART0_FLOW_CONTROL (HAL_USART_FLOW_CONTROL_HWUART)
// [USART0]$

// $[USART1]
#define PORTIO_USART1_CLK_PIN                         (8U)
#define PORTIO_USART1_CLK_PORT                        (gpioPortC)
#define PORTIO_USART1_CLK_LOC                         (11U)

#define PORTIO_USART1_CS_PIN                          (9U)
#define PORTIO_USART1_CS_PORT                         (gpioPortC)
#define PORTIO_USART1_CS_LOC                          (11U)

#define PORTIO_USART1_RX_PIN                          (7U)
#define PORTIO_USART1_RX_PORT                         (gpioPortC)
#define PORTIO_USART1_RX_LOC                          (11U)

#define PORTIO_USART1_TX_PIN                          (6U)
#define PORTIO_USART1_TX_PORT                         (gpioPortC)
#define PORTIO_USART1_TX_LOC                          (11U)

#define BSP_USART1_MOSI_PIN                           (6U)
#define BSP_USART1_MOSI_PORT                          (gpioPortC)
#define BSP_USART1_MOSI_LOC                           (11U)

#define BSP_USART1_MISO_PIN                           (7U)
#define BSP_USART1_MISO_PORT                          (gpioPortC)
#define BSP_USART1_MISO_LOC                           (11U)

#define BSP_USART1_CLK_PIN                            (8U)
#define BSP_USART1_CLK_PORT                           (gpioPortC)
#define BSP_USART1_CLK_LOC                            (11U)

#define BSP_USART1_CS_PIN                             (9U)
#define BSP_USART1_CS_PORT                            (gpioPortC)
#define BSP_USART1_CS_LOC                             (11U)

// [USART1]$

// $[VCOM]

#define BSP_VCOM_ENABLE_PIN                           (5U)
#define BSP_VCOM_ENABLE_PORT                          (gpioPortA)

// [VCOM]$

// $[VUART]
// [VUART]$

// $[WDOG]
// [WDOG]$

#endif /* HAL_CONFIG_H */
