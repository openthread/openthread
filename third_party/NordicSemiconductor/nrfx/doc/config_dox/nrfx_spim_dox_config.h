/**
 *
 * @defgroup nrfx_spim_config SPIM peripheral driver configuration
 * @{
 * @ingroup nrfx_spim
 */
/** @brief 
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM_ENABLED
/** @brief Enable SPIM0 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM0_ENABLED

/** @brief Enable SPIM1 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM1_ENABLED

/** @brief Enable SPIM2 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM2_ENABLED

/** @brief Enable SPIM3 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM3_ENABLED

/** @brief Enable extended SPIM features
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM_EXTENDED_ENABLED

/** @brief MISO pin pull configuration.
 *
 *  Following options are available:
 * - 0 - NRF_GPIO_PIN_NOPULL
 * - 1 - NRF_GPIO_PIN_PULLDOWN
 * - 3 - NRF_GPIO_PIN_PULLUP
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM_MISO_PULL_CFG

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
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM_CONFIG_LOG_ENABLED
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
#define NRFX_SPIM_CONFIG_LOG_LEVEL

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
#define NRFX_SPIM_CONFIG_INFO_COLOR

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
#define NRFX_SPIM_CONFIG_DEBUG_COLOR


/** @brief Enables nRF52 anomaly 109 workaround for SPIM.
 *
 * The workaround uses interrupts to wake up the CPU by catching
 * a start event of zero-length transmission to start the clock. This 
 * ensures that the DMA transfer will be executed without issues and
 * that the proper transfer will be started. See more in the Errata 
 * document or Anomaly 109 Addendum located at 
 * https://infocenter.nordicsemi.com/
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED

/** @brief Enables nRF52840 anomaly 198 workaround for SPIM3.
 *
 * See more in the Errata document located at 
 * https://infocenter.nordicsemi.com/
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SPIM3_NRF52840_ANOMALY_198_WORKAROUND_ENABLED


/** @} */
