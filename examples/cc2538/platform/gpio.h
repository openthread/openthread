#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "cc2538-reg.h"

#ifdef __cplusplus
 extern "C" {
#endif

enum {
  GPIO_A_NUM = 0,
  GPIO_B_NUM = 1,
  GPIO_C_NUM = 2,
  GPIO_D_NUM = 3,
};

// port dev
#define GPIO_A_DEV              0x400d9000
#define GPIO_B_DEV              0x400da000
#define GPIO_C_DEV              0x400db000
#define GPIO_D_DEV              0x400dc000

// helper macros
#define GPIO_PIN_MASK(n)        ( 1 << (n) )
#define GPIO_PORT_TO_DEV(port)  (GPIO_A_DEV + ((port) << 12))

// in/out control
#define IOC_PXX_SEL             0x400d4000
#define IOC_PXX_OVER            0x400d4080

// peripheral select values
#define IOC_SEL_UART0_TXD       (0)
#define IOC_SEL_UART1_RTS       (1)
#define IOC_SEL_UART1_TXD       (2)
#define IOC_SEL_SSI0_TXD        (3)
#define IOC_SEL_SSI0_CLKOUT     (4)
#define IOC_SEL_SSI0_FSSOUT     (5)
#define IOC_SEL_SSI0_STXSER_EN  (6)
#define IOC_SEL_SSI1_TXD        (7)
#define IOC_SEL_SSI1_CLKOUT     (8)
#define IOC_SEL_SSI1_FSSOUT     (9)
#define IOC_SEL_SSI1_STXSER_EN  (10)
#define IOC_SEL_I2C_CMSSDA      (11)
#define IOC_SEL_I2C_CMSSCL      (12)
#define IOC_SEL_GPT0_ICP1       (13)
#define IOC_SEL_GPT0_ICP2       (14)
#define IOC_SEL_GPT1_ICP1       (15)
#define IOC_SEL_GPT1_ICP2       (16)
#define IOC_SEL_GPT2_ICP1       (17)
#define IOC_SEL_GPT2_ICP2       (18)
#define IOC_SEL_GPT3_ICP1       (19)
#define IOC_SEL_GPT3_ICP2       (20)

// GPIO offsets
#define GPIO_DATA               0x00000000
#define GPIO_DIR                0x00000400
#define GPIO_IS                 0x00000404
#define GPIO_IBE                0x00000408
#define GPIO_IEV                0x0000040C
#define GPIO_IE                 0x00000410
#define GPIO_RIS                0x00000414
#define GPIO_MIS                0x00000418
#define GPIO_IC                 0x0000041C
#define GPIO_AFSEL              0x00000420
#define GPIO_GPIOLOCK           0x00000520
#define GPIO_GPIOCR             0x00000524
#define GPIO_PMUX               0x00000700
#define GPIO_P_EDGE_CTRL        0x00000704
#define GPIO_USB_CTRL           0x00000708
#define GPIO_PI_IEN             0x00000710
#define GPIO_IRQ_DETECT_ACK     0x00000718
#define GPIO_USB_IRQ_ACK        0x0000071C
#define GPIO_IRQ_DETECT_UNMASK  0x00000720

// GPIO macros
#define cc2538GpioHardwareControl(port_num, pin_num) ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_AFSEL) |= GPIO_PIN_MASK(pin_num) )
#define cc2538GpioSoftwareControl(port_num, pin_num) ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_AFSEL) &= ~GPIO_PIN_MASK(pin_num) )
#define cc2538GpioDirOutput(port_num, pin_num)       ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_DIR) |= GPIO_PIN_MASK(pin_num) )
#define cc2538GpioDirInput(port_num, pin_num)        ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_DIR) &= ~GPIO_PIN_MASK(pin_num) )
#define cc2538GpioReadPin(port_num, pin_num)         ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_DATA + (GPIO_PIN_MASK(pin_num) << 2)) )
#define cc2538GpioSetPin(port_num, pin_num)          ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_DATA + (GPIO_PIN_MASK(pin_num) << 2)) = 0xFF )
#define cc2538GpioClearPin(port_num, pin_num)        ( HWREG(GPIO_PORT_TO_DEV(port_num) + GPIO_DATA + (GPIO_PIN_MASK(pin_num) << 2)) = 0x00 )

// IOC functions
void cc2538GpioIocOver(uint8_t port, uint8_t pin, uint8_t over);
void cc2538GpioIocSel(uint8_t port, uint8_t pin, uint8_t sel);

#ifdef __cplusplus
} // end extern "C"
#endif
#endif // GPIO_H_
