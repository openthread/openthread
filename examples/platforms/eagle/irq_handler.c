#include "platform-eagle.h"
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief exception handler.
 *
 * This defines an exception handler to handle all the platform pre-defined
 * exceptions.
 ****************************************************************************************
 */
void except_handler0(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void except_handler0(void) {
}

/**
 ****************************************************************************************
 * @brief System timer interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler1(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler1(void) {
}

/**
 ****************************************************************************************
 * @brief alg interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler2(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler2(void) {
}

/**
 ****************************************************************************************
 * @brief timer1 interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */

void interrupt_handler3(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler3(void) {

}

/**
 ****************************************************************************************
 * @brief timer0 interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */

void interrupt_handler4(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler4(void) {

}

/**
 ****************************************************************************************
 * @brief dma interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler5(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler5(void) {}

/**
 ****************************************************************************************
 * @brief bmc interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler6(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler6(void) {
}

/**
 ****************************************************************************************
 * @brief udc[0] interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler7(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler7(void) {
}

/**
 ****************************************************************************************
 * @brief udc[1] interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler8(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler8(void) {
}

/**
 ****************************************************************************************
 * @brief udc[2] interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler9(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler9(void) {
}

/**
 ****************************************************************************************
 * @brief udc[3] interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler10(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler10(void) {
}

/**
 ****************************************************************************************
 * @brief udc[4] interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler11(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler11(void) {
}

/**
 ****************************************************************************************
 * @brief zb_dm interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler12(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler12(void) {}

/**
 ****************************************************************************************
 * @brief zb_ble interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler13(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler13(void) {}

/**
 ****************************************************************************************
 * @brief zb_bt interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler14(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler14(void) {}

/**
 ****************************************************************************************
 * @brief zb_rt interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler15(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler15(void) {
    EagleRxTxIntHandler();
}

/**
 ****************************************************************************************
 * @brief pwm interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler16(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler16(void) {
}

/**
 ****************************************************************************************
 * @brief uart1 interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler17(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler17(void) {
}

/**
 ****************************************************************************************
 * @brief uart0 interrupt handler.
 *
 * This defines an interrupt handler to handle the interrupt with the number defined
 *  by the INTCNTL_BT macro.
 ****************************************************************************************
 */
void interrupt_handler18(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler18(void) {
}

void interrupt_handler19(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler19(void) {
    irq_uart0_handler();
}

void interrupt_handler20(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler20(void) {
}

void interrupt_handler21(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler21(void) {
}

void interrupt_handler22(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler22(void) {}

void interrupt_handler23(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler23(void) {}

void interrupt_handler24(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler24(void) {}

void interrupt_handler25(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler25(void) {}

void interrupt_handler26(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler26(void) {


}

void interrupt_handler27(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler27(void) {



}

void interrupt_handler28(void) __attribute__ ((interrupt ("machine"), aligned(4)));
void interrupt_handler28(void) {}

/// @} DRIVERS
