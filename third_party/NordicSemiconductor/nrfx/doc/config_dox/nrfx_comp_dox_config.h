/**
 *
 * @defgroup nrfx_comp_config COMP peripheral driver configuration
 * @{
 * @ingroup nrfx_comp
 */
/** @brief Enable COMP driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_ENABLED
/** @brief Reference voltage
 *
 *  Following options are available:
 * - 0 - Internal 1.2V
 * - 1 - Internal 1.8V
 * - 2 - Internal 2.4V
 * - 4 - VDD
 * - 7 - ARef
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_REF

/** @brief Main mode
 *
 *  Following options are available:
 * - 0 - Single ended
 * - 1 - Differential
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_MAIN_MODE

/** @brief Speed mode
 *
 *  Following options are available:
 * - 0 - Low power
 * - 1 - Normal
 * - 2 - High speed
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_SPEED_MODE

/** @brief Hystheresis
 *
 *  Following options are available:
 * - 0 - No
 * - 1 - 50mV
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_HYST

/** @brief Current Source
 *
 *  Following options are available:
 * - 0 - Off
 * - 1 - 2.5 uA
 * - 2 - 5 uA
 * - 3 - 10 uA
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_ISOURCE

/** @brief Analog input
 *
 *  Following options are available:
 * - 0
 * - 1
 * - 2
 * - 3
 * - 4
 * - 5
 * - 6
 * - 7
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_INPUT

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
#define NRFX_COMP_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_COMP_CONFIG_LOG_ENABLED
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
#define NRFX_COMP_CONFIG_LOG_LEVEL

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
#define NRFX_COMP_CONFIG_INFO_COLOR

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
#define NRFX_COMP_CONFIG_DEBUG_COLOR



/** @} */
