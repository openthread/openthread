#ifndef SYSTEM_JN518X_H
#define SYSTEM_JN518X_H

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t SystemCoreClock;     /*!< System Clock Frequency (Core Clock)  */


/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
extern void SystemInit (void);

/**
 * Update SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
extern void SystemCoreClockUpdate (void);



/* *** JN518x *** */

/** \brief  Exceptions and Interrupts vectors structure.
 */

typedef struct
{
   uint32_t pExceptions[16]; /*!< System exceptions vectors */
   uint32_t pIsr[240];       /*!< User interrupt vectors    */
} VectorTable_Type;


extern const uint32_t  XSW_VTOR_BASE        ;

#define JN518X_ISR_TABLE   ((VectorTable_Type *)     XSW_VTOR_BASE)

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_JN518X_H */
