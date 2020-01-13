/**
 *
 * @defgroup nrfx_qspi_config QSPI peripheral driver configuration
 * @{
 * @ingroup nrfx_qspi
 */
/** @brief Enable QSPI driver.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_ENABLED
/** @brief tSHSL, tWHSL and tSHWL in number of 16 MHz periods (62.5 ns).
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_SCK_DELAY

/** @brief Address offset in the external memory for Execute in Place operation.
 *
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_XIP_OFFSET

/** @brief Number of data lines and opcode used for reading.
 *
 *  Following options are available:
 * - 0 - FastRead
 * - 1 - Read2O
 * - 2 - Read2IO
 * - 3 - Read4O
 * - 4 - Read4IO
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_READOC

/** @brief Number of data lines and opcode used for writing.
 *
 *  Following options are available:
 * - 0 - PP
 * - 1 - PP2O
 * - 2 - PP4O
 * - 3 - PP4IO
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_WRITEOC

/** @brief Addressing mode.
 *
 *  Following options are available:
 * - 0 - 24bit
 * - 1 - 32bit
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_ADDRMODE

/** @brief SPI mode.
 *
 *  Following options are available:
 * - 0 - Mode 0
 * - 1 - Mode 1
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_MODE

/** @brief Frequency divider.
 *
 *  Following options are available:
 * - 0 - 32MHz/1
 * - 1 - 32MHz/2
 * - 2 - 32MHz/3
 * - 3 - 32MHz/4
 * - 4 - 32MHz/5
 * - 5 - 32MHz/6
 * - 6 - 32MHz/7
 * - 7 - 32MHz/8
 * - 8 - 32MHz/9
 * - 9 - 32MHz/10
 * - 10 - 32MHz/11
 * - 11 - 32MHz/12
 * - 12 - 32MHz/13
 * - 13 - 32MHz/14
 * - 14 - 32MHz/15
 * - 15 - 32MHz/16
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_FREQUENCY

/** @brief SCK pin value.
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_PIN_SCK

/** @brief CSN pin value.
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_PIN_CSN

/** @brief IO0 pin value.
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_PIN_IO0

/** @brief IO1 pin value.
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_PIN_IO1

/** @brief IO2 pin value.
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_PIN_IO2

/** @brief IO3 pin value.
 *
 *  Minimum value: 0
 *  Maximum value: 255
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_PIN_IO3

/** @brief Interrupt priority
 *
 *  Following options are available:
 * - 0 - 0 (highest)
 * - 1 - 1
 * - 2 - 2
 * - 3 - 3
 * - 4 - 4
 * - 5 - 5
 * - 6 - 6
 * - 7 - 7
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_QSPI_CONFIG_IRQ_PRIORITY


/** @} */
