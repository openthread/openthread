/**
 *
 * @defgroup nrfx_twis_config TWIS peripheral driver configuration
 * @{
 * @ingroup nrfx_twis
 */
/** @brief 
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_ENABLED
/** @brief Enable TWIS0 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS0_ENABLED

/** @brief Enable TWIS1 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS1_ENABLED

/** @brief Assume that any instance would be initialized only once
 *
 * Optimization flag. Registers used by TWIS are shared by other peripherals. Normally, during initialization driver tries to clear all registers to known state before doing the initialization itself. This gives initialization safe procedure, no matter when it would be called. If you activate TWIS only once and do never uninitialize it - set this flag to 1 what gives more optimal code.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY

/** @brief Remove support for synchronous mode
 *
 * Synchronous mode would be used in specific situations. And it uses some additional code and data memory to safely process state machine by polling it in status functions. If this functionality is not required it may be disabled to free some resources.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_NO_SYNC_MODE

/** @brief Address0
 *
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_DEFAULT_CONFIG_ADDR0

/** @brief Address1
 *
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_DEFAULT_CONFIG_ADDR1

/** @brief SCL pin pull configuration
 *
 *  Following options are available:
 * - 0 - Disabled
 * - 1 - Pull down
 * - 3 - Pull up
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_DEFAULT_CONFIG_SCL_PULL

/** @brief SDA pin pull configuration
 *
 *  Following options are available:
 * - 0 - Disabled
 * - 1 - Pull down
 * - 3 - Pull up
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_DEFAULT_CONFIG_SDA_PULL

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
#define NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_CONFIG_LOG_ENABLED
/** @brief Default Severity level
 *
 *  Following options are available:
 * - 0 - Off
 * - 1 - Error
 * - 2 - Warning
 * - 3 - Info
 * - 4 - Debug
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_CONFIG_LOG_LEVEL

/** @brief ANSI escape code prefix.
 *
 *  Following options are available:
 * - 0 - Default
 * - 1 - Black
 * - 2 - Red
 * - 3 - Green
 * - 4 - Yellow
 * - 5 - Blue
 * - 6 - Magenta
 * - 7 - Cyan
 * - 8 - White
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_CONFIG_INFO_COLOR

/** @brief ANSI escape code prefix.
 *
 *  Following options are available:
 * - 0 - Default
 * - 1 - Black
 * - 2 - Red
 * - 3 - Green
 * - 4 - Yellow
 * - 5 - Blue
 * - 6 - Magenta
 * - 7 - Cyan
 * - 8 - White
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIS_CONFIG_DEBUG_COLOR



/** @} */
